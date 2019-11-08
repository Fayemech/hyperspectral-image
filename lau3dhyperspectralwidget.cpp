/*********************************************************************************
 *                                                                               *
 * Copyright (c) 2017, Dr. Daniel L. Lau                                         *
 * All rights reserved.                                                          *
 *                                                                               *
 * Redistribution and use in source and binary forms, with or without            *
 * modification, are permitted provided that the following conditions are met:   *
 * 1. Redistributions of source code must retain the above copyright             *
 *    notice, this list of conditions and the following disclaimer.              *
 * 2. Redistributions in binary form must reproduce the above copyright          *
 *    notice, this list of conditions and the following disclaimer in the        *
 *    documentation and/or other materials provided with the distribution.       *
 * 3. All advertising materials mentioning features or use of this software      *
 *    must display the following acknowledgement:                                *
 *    This product includes software developed by the <organization>.            *
 * 4. Neither the name of the <organization> nor the                             *
 *    names of its contributors may be used to endorse or promote products       *
 *    derived from this software without specific prior written permission.      *
 *                                                                               *
 * THIS SOFTWARE IS PROVIDED BY Dr. Daniel L. Lau ''AS IS'' AND ANY              *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE        *
 * DISCLAIMED. IN NO EVENT SHALL Dr. Daniel L. Lau BE LIABLE FOR ANY             *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND   *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                  *
 *                                                                               *
 *********************************************************************************/

#include "lau3dhyperspectralwidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DHyperspectralRecordingWidget::LAU3DHyperspectralRecordingWidget(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : LAU3DVideoWidget(color, device, parent), videoLabel(nullptr), snapShotModeFlag(false), videoRecordingFlag(false), velmexWidget(nullptr), previewGLWidget(nullptr)
{
    if (camera && camera->isValid()) {
        // ALLOCATE FRAME BUFFER MANAGER TO MANAGE MEMORY FOR US
        frameBufferManager = new LAUMemoryObjectManager(camera->width(), camera->height(), this->colors(), sizeof(float), 1, nullptr);

        // CONNECT SIGNALS AND SLOTS BETWEEN THIS AND THE FRAME BUFFER MANAGER
        connect(this, SIGNAL(emitGetFrame()), frameBufferManager, SLOT(onGetFrame()), Qt::QueuedConnection);
        connect(this, SIGNAL(emitReleaseFrame(LAUMemoryObject)), frameBufferManager, SLOT(onReleaseFrame(LAUMemoryObject)), Qt::QueuedConnection);
        connect(frameBufferManager, SIGNAL(emitFrame(LAUMemoryObject)), this, SLOT(onReceiveFrameBuffer(LAUMemoryObject)), Qt::QueuedConnection);

        // SPIN THE FRAME BUFFER MANAGER INTO ITS OWN THREAD
        frameBufferManagerController = new LAUController(frameBufferManager);

        // NOW ASK THE FRAME BUFFER MANAGER TO GIVE US A SMALL STASH OF 10 OR SO FRAMES
        for (int n = videoFramesBufferList.count(); n < 4; n++) {
            emit emitGetFrame();
        }

        // CREATE A PREVIEW WIDGET TO SHOW THE SCAN AS ITS BEING RECORDED
        previewGLWidget = new LAU3DHyperspectralGLWidget(camera->width(), camera->height(), ColorGray, this);
        previewGLWidget->setWindowFlag(Qt::Tool);
        connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), previewGLWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(this, SIGNAL(emitResetFilter()), previewGLWidget, SLOT(onReset()));

        // CREATE A CONTEXT MENU ACTION FOR DISPLAYING THE VELMEX RAIL
        QAction *action = new QAction(QString("Show Preview widget..."), nullptr);
        action->setCheckable(false);
        connect(action, SIGNAL(triggered()), this, SLOT(onShowPreviewWidget()));
        this->insertAction(action);
    }
    videoLabel = new LAUVideoPlayerLabel(LAUVideoPlayerLabel::StateVideoRecorder);
    connect(videoLabel, SIGNAL(playButtonClicked(bool)), this, SLOT(onRecordButtonClicked(bool)), Qt::QueuedConnection);
    this->layout()->addWidget(videoLabel);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DHyperspectralRecordingWidget::~LAU3DHyperspectralRecordingWidget()
{
    // DELETE ALL ACCUMULATED VIDEO FRAME BUFFERS
    while (videoFramesBufferList.isEmpty() == false) {
        emit emitReleaseFrame(videoFramesBufferList.takeFirst());
    }

    // DELETE THE VELMEX WIDGET IF IT EXISTS
    if (velmexWidget) {
        delete velmexWidget;
    }

    // DELETE THE PREVIEW WIDGET IF IT EXISTS
    if (previewGLWidget) {
        delete previewGLWidget;
    }

    // CLEAR THE RECORDED VIDEO FRAMES LIST
    recordedVideoFramesBufferList.clear();

    qDebug() << QString("LAU3DHyperspectralRecordingWidget::~LAU3DHyperspectralRecordingWidget()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::onRecordButtonClicked(bool state)
{
    // TELL THE TRACKING FILTER TO RESET THE ANCHOR FRAME
    if (state){
        emit emitResetFilter();
    }

    if (camera && camera->isValid()) {
#ifndef IDS
        if (snapShotModeFlag) {
            if (state) {
                // GRAB A COPY OF THE PACKET WE INTEND TO COPY INTO
                LAUMemoryObject packet = getPacket();

                // COPY SCAN FROM GLWIDGET AND SEND TO VIDEO SINK OBJECT RUNNING IN A SEPARATE THREAD
                if (glWidget) {
                    glWidget->copyScan((float *)packet.pointer());
                }

                // PRESERVE THE TIME ELAPSED
                packet.setElapsed(timeStamp.elapsed());

                // EMIT THE VIDEO FRAME
                emit emitVideoFrames(packet);

                // TELL THE VELMEX RAIL THAT WE ARE READY TO MOVE TO THE NEXT POSITION
                if (velmexIteration > -1 && velmexNumberOfIterations > -1) {
                    emit emitTriggerScanner(0.0f, velmexIteration, velmexNumberOfIterations);
                }

                // STOP THE FRAME RECORDING
                videoLabel->onPlayButtonClicked(false);
            }
        } else {
#endif
            videoRecordingFlag = state;
            if (state) {
                // RESET RECORDING FRAME COUNTER AND TIMER AND WE CAN JUST DUMP
                // THE INCOMING VIDEO TO OUR VIDEO FRAME BUFFER LIST
                pressStartButtonTime = QTime::currentTime();
                timeStamp.restart();

                // TELL THE VELMEX RAIL THAT WE ARE READY TO MOVE TO THE NEXT POSITION
                if (velmexIteration > -1 && velmexNumberOfIterations > -1) {
                    emit emitTriggerScanner(0.0f, velmexIteration, velmexNumberOfIterations);
                }
#ifndef IDS
            } else {
                // EMIT THE LIST OF RECORDED FRAMES OUT TO A RECEIVER OBJECT
                emit emitVideoFrames(recordedVideoFramesBufferList);

                // NOW THAT THE RECIEVER OBJECT HAS THE LIST, WE CAN DELETE IT
                recordedVideoFramesBufferList.clear();

                // RESET THE PROGRESS BAR TO SHOW NO VIDEO IS CURRENTLY RECORDED
                videoLabel->onUpdateSliderPosition(0.0f);
                videoLabel->onUpdateTimeStamp(0);
            }
#endif
        }
    } else {
        if (state) {
            videoLabel->onPlayButtonClicked(false);
            QMessageBox::warning(this, QString("Video Recorded Widget"), QString("No device available."));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::onReceiveVideoFrames(LAUMemoryObject frame)
{
    // CREATE A LAUSCAN TO HOLD THE INCOMING SNAP SHOT
    LAUScan scan(frame, playbackColor);
    if (scan.isValid()) {
        scan.updateLimits();
        scan.setSoftware(QString("Lau 3D Video Recorder"));
        scan.setMake(camera->make());
        scan.setModel(camera->model());

        // ASK THE USER TO APPROVE THE SCAN BEFORE SAVING TO DISK
        while (scan.approveImage()) {
            // NOW LET THE USER SAVE THE SCAN TO DISK
            if (scan.save() == true) {
                break;
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::onReceiveVideoFrames(QList<LAUMemoryObject> frameList)
{
    // NOW WE NEED TO PASS OUR VIDEO FRAMES TO A DEPTH VIDEO OBJECT
    // WHICH CAN THEN BRING UP A VIDEO PLAYER WIDGET TO REPLAYING
    // THE VIDEO ON SCREEN, AND GIVE THE USER THE CHANCE TO SAVE TO DISK
    if (frameList.count() > 0) {
        // CREATE THE VIDEO PLAYER WIDGET AND SEND IT THE VIDEO FRAMES
        LAU3DVideoPlayerWidget *replayWidget = new LAU3DVideoPlayerWidget(camera->width(), camera->height(), playbackColor, this);

        // DISPLAY THE REPLAY WIDGET SO THE USER CAN SEE THE FINE MERGE'S PROGRESS
        // SEND THE RECORDED FRAME PACKETS TO OUR REPLAY VIDEO WIDGET FOR PLAYBACK
        while (!frameList.isEmpty()) {
            replayWidget->onInsertPacket(frameList.takeFirst());
        }

        // GET THE VIEW LIMITS OF THE GPU WIDGET AND COPY THEM TO THE REPLAY WIDGET
        QVector2D xLimits = glWidget->xLimits();
        QVector2D yLimits = glWidget->yLimits();
        QVector2D zLimits = glWidget->zLimits();

        replayWidget->setLimits(xLimits.x(), xLimits.y(), yLimits.x(), yLimits.y(), zLimits.x(), zLimits.y());
        replayWidget->setAttribute(Qt::WA_DeleteOnClose);
        replayWidget->show();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::onReceiveFrameBuffer(LAUMemoryObject buffer)
{
    videoFramesBufferList << buffer;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAU3DHyperspectralRecordingWidget::getPacket()
{
    LAUMemoryObject packet;
    if (!videoFramesBufferList.isEmpty()) {
        packet = videoFramesBufferList.takeFirst();
    }
    for (int n = videoFramesBufferList.count(); n < 4; n++) {
        emit emitGetFrame();
    }
    return (packet);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::releasePacket(LAUMemoryObject packet)
{
    videoFramesBufferList << packet;
    while (videoFramesBufferList.count() > 4) {
        emit emitReleaseFrame(videoFramesBufferList.takeFirst());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // UPDATE THE TEXTURE BUFFERS IF WE HAVE AT LEAST ONE VALID POINTER
    if (depth.isValid() || color.isValid() || mapping.isValid()) {
        if (videoRecordingFlag) {
#ifdef IDS
            // FOR IDS, WE ONLY PERFORM SNAP SHOT RECORDING
            videoLabel->onPlayButtonClicked(false);

            // FOR IDS RECORDING, SEND THE MEMORY OBJECT TO A SPECIAL SAVE METHOD
            // HOSTED AS A STATIC METHOD BY THE LAUMSCOLORHISTOGRAMGLFILTER CLASS
            LAUMSColorHistogramGLFilter::save(color);
#else
            // NOW THE BUFFER IS FULL, TELL VIDEO LABEL TO STOP RECORDING
            if (recordedVideoFramesBufferList.count() < MAXRECORDEDFRAMECOUNT) {
                // GRAB A COPY OF THE PACKET WE INTEND TO COPY INTO
                LAUMemoryObject packet = getPacket();

                // COPY SCAN FROM GLWIDGET AND SEND TO VIDEO SINK OBJECT RUNNING IN A SEPARATE THREAD
                if (glWidget) {
                    glWidget->copyScan((float *)packet.pointer());
                }

                // PRESERVE THE TIME ELAPSED
                packet.setElapsed(timeStamp.elapsed());

                // HAVE THE VIDEO LABEL UPDATE ITS PROGRESS BAR SO THE USER KNOWS HOW MUCH SPACE IS LEFT
                videoLabel->onUpdateSliderPosition((float)recordedVideoFramesBufferList.count() / (float)MAXRECORDEDFRAMECOUNT);
                videoLabel->onUpdateTimeStamp(packet.elapsed());

                // ADD THE INCOMING PACKET TO OUR RECORDED FRAME BUFFER LIST
                recordedVideoFramesBufferList << packet;
            } else {
                // NOW THE BUFFER IS FULL, TELL VIDEO LABEL TO STOP RECORDING
                videoLabel->onPlayButtonClicked(false);
            }
#endif
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::onTriggerScanner(float pos, int n, int N)
{
    Q_UNUSED(pos);

    if (snapShotModeFlag == true) {
        if (n < 0) {
            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = n;
            velmexNumberOfIterations = N;
        } else if (n >= N) {
            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = -1;
            velmexNumberOfIterations = -1;
        } else {
            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = n;
            velmexNumberOfIterations = N;

            // TRIGGER A SCAN
            this->onRecordButtonClicked(true);
        }
    } else {
        if (n < 0) {
            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = n;
            velmexNumberOfIterations = N;
        } else if (n >= N) {
            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = -1;
            velmexNumberOfIterations = -1;

            // TRIGGER A SCAN
            this->onRecordButtonClicked(false);
        } else {
            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = n;
            velmexNumberOfIterations = N;

            // TRIGGER A SCAN
            this->onRecordButtonClicked(true);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::enableVelmexScanMode(bool state)
{
    if (state && velmexWidget == nullptr) {
        // CREATE VELMEX RAIL WIDGET TO CONTROL RAIL
        velmexWidget = new LAUVelmexWidget(0, nullptr, this);
        if (velmexWidget->isValid()) {
            // SEE IF WE SHOULD BE SCANNING WHILE MOVING
            velmexWidget->enableMovingScanMode(!snapShotModeFlag);
            velmexWidget->setWindowFlag(Qt::Tool);
            connect(velmexWidget, SIGNAL(emitTriggerScanner(float, int, int)), this, SLOT(onTriggerScanner(float, int, int)), Qt::QueuedConnection);
            connect(this, SIGNAL(emitTriggerScanner(float, int, int)), velmexWidget, SLOT(onTriggerScanner(float, int, int)), Qt::QueuedConnection);

            // ENABLE THE WIDGET SO THAT THE USER CAN INTERACT WITH IT
            velmexWidget->setEnabled(true);

            // CREATE A CONTEXT MENU ACTION FOR DISPLAYING THE VELMEX RAIL
            QAction *action = new QAction(QString("Show Velmex rail controller..."), nullptr);
            action->setCheckable(false);
            connect(action, SIGNAL(triggered()), this, SLOT(onShowVelmexWidget()));
            this->insertAction(action);
        } else {
            // DISABLE THE WIDGET SO THAT THE USER CANNOT INTERACT WITH IT
            velmexWidget->hide();
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::onShowVelmexWidget()
{
    // HIDE AND THEN SHOW THE VELMEX WIDGET
    // SO IT ENDS UP AS THE TOP WINDOW
    if (velmexWidget) {
        velmexWidget->hide();
        velmexWidget->show();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DHyperspectralRecordingWidget::onShowPreviewWidget()
{
    // HIDE AND THEN SHOW THE VELMEX WIDGET
    // SO IT ENDS UP AS THE TOP WINDOW
    if (previewGLWidget) {
        previewGLWidget->hide();
        previewGLWidget->show();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAU3DHyperspectralRecordingWidget::processFrames(QList<LAUScan> frameList)
{
    int numRows = static_cast<int>(frameList.first().height());
    int numCols = static_cast<int>(frameList.first().width());
    int numFrms = frameList.count();

    // CENTER LINE HEIGHT WE WANT, ODD NUMBER IS BETTER
    int numLnes = 3;

    // SET THE ANCHOR IMAGE AS THE MIDDLE FRAME, SAVE THE CENTERLINE FOR LATER USE
    cv::Mat anchorMat = scanToMatrix(frameList.at(numFrms / 2));

    cv::Rect roiB(0, numRows / 2, numCols, numRows / 2);
    cv::Rect roiB3(0, (numRows * 3 / 4) - (numLnes - 1) / 2, numCols, numLnes);

    cv::Mat centerLine = anchorMat(roiB3);
    cv::Mat imgMid = anchorMat(roiB);
    cv::Mat panorama(numRows / 2, numCols, CV_8UC1);
    for (int i = 0; i < numLnes; i++) {
        uchar* imgLine = centerLine.ptr<uchar>(i);
        for (int j = 0; j < numCols; j++) {
            panorama.at<uchar>((numRows * 1 / 4) - (numLnes - 1) / 2 + i, j) = imgLine[j];
        }
    }
    std::vector<cv::KeyPoint> pointsPrev;
    cv::Mat desPrev;
    cv::Ptr<cv::ORB> Orb = cv::ORB::create();
    Orb->detectAndCompute(imgMid, cv::Mat(), pointsPrev, desPrev);

    QProgressDialog progressDialog(QString("Merging frames..."), QString(), 0, numFrms, nullptr, Qt::Sheet);
    for (int n = 0; n < numFrms; n++) {
        progressDialog.setValue(n);
        qApp->processEvents();

        // GET THE NEXT AVAILABLE FRAME FROM THE LIST OF FRAMES
        cv::Mat frame = scanToMatrix(frameList.at(n));

        // MOVE THE OLD FEATURES TO THE PREV TO READY FOR THE NEW IMG
        std::vector<cv::KeyPoint> pointsCur;
        cv::Mat desCur;
        cv::Mat imgCur = frame(roiB);
        cv::Mat centerLineCur = frame(roiB3);

        // WE WILL SUM THE CENTERLINE LATER
        Orb->detectAndCompute(imgCur, cv::Mat(), pointsCur, desCur);

        //NOW WE HAVE TWO SET OF FEATURES, MATCH THEM
        std::vector<cv::DMatch> match1to2, match2to1, matchGood;
        cv::Ptr<cv::DescriptorMatcher> Matcher = cv::DescriptorMatcher::create("BruteForce");
        Matcher->match(desPrev, desCur, match1to2);
        Matcher->match(desCur, desPrev, match2to1);

        //PICK OUT GOOD MATCH PAIR
        int *Flag = new int[desCur.rows];
        memset(Flag, -1, sizeof(int) * desCur.rows);

        for (size_t i = 0; i < desCur.rows; i++) {
            Flag[match2to1[i].queryIdx] = match2to1[i].trainIdx;
        }

        for (size_t i = 0; i < match1to2.size(); i++) {
            if (Flag[match1to2[i].trainIdx] == match1to2[i].queryIdx) {
                matchGood.push_back(match1to2[i]);
            }
        }

        //50 PAIR IS ENOUGH, IF ITS SMALLER, WE STILL ACCPET IT
        sort(matchGood.begin(), matchGood.end());
        size_t loop = matchGood.size() * 0.15 > 50 ? 50 : matchGood.size() * 0.15;
        std::vector<cv::DMatch> Res;
        for (int i = 0; i < loop; i++) {
            Res.push_back(matchGood[i]);
        }

        std::vector<cv::Point2f> imgPoints1, imgPoints2;
        for (int i = 0; i < Res.size(); i++) {
            imgPoints1.push_back(pointsPrev[Res[i].queryIdx].pt);
            imgPoints2.push_back(pointsCur[Res[i].trainIdx].pt);
        }

        // CALCULATE THE HOMOGRAPHY FROM FRAME TO ANCHOR
        cv::Mat homoMatrix;
        if (imgPoints1.size() < 4) {
            continue;
        } else {
            homoMatrix = findHomography(imgPoints2, imgPoints1, cv::RANSAC);
            //NOW WE HAVE ALL THE HOMOMATRIX, DO THE STITCHING
            for (int i = 0; i < numLnes; i++) {
                uchar* imgLine = centerLineCur.ptr<uchar>(i);
                for (int j = 0; j < numCols; j++) {
                    cv::Mat cor, corRes;
                    cor = (cv::Mat_<double>(3, 1) << j, (numRows * 1 / 4) - (numLnes - 1) / 2 + i, 1);
                    corRes = homoMatrix * cor;
                    double colCur = corRes.at<double>(0, 0);
                    double rowCur = corRes.at<double>(1, 0);
                    double divFactor = corRes.at <double>(2, 0);
                    int c1 = floor(colCur / divFactor);
                    int r1 = floor(rowCur / divFactor);
                    if (c1 > 0 && r1 > 0 && c1 < numCols && r1 < numRows / 2) {
                        panorama.at<uchar>(r1, c1) = imgLine[j];
                    }
                }
            }
        }
    }
    progressDialog.setValue(numFrms);

    // CONVERT CV::MAT INTO FLOATING POINT FORMAT
    panorama.convertTo(panorama, CV_32FC1);
    panorama = panorama / 255.0f;

    LAUScan output(panorama.cols, panorama.rows, ColorGray);
    for (int row = 0; row < panorama.rows; row++){
        memcpy(output.constScanLine(row), panorama.ptr(row), output.step());
    }

    return(output);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
cv::Mat LAU3DHyperspectralRecordingWidget::scanToMatrix(LAUScan scn)
{
    int numRows = static_cast<int>(scn.height());
    int numCols = static_cast<int>(scn.width());
    int numChns = static_cast<int>(scn.colors());
    int numByts = static_cast<int>(scn.depth());

    cv::Mat image;
    if (numChns == 1){
        // GRAYSCALE VIDEO
        if (numByts == sizeof(unsigned char)){
            image = cv::Mat(numRows, numCols, CV_8UC1, scn.constPointer(), scn.step());
        } else if (numByts == sizeof(unsigned short)){
            image = cv::Mat(numRows, numCols, CV_16UC1, scn.constPointer(), scn.step());
            image.convertTo(image, CV_8UC1, 255);
        } else if (numByts == sizeof(float)){
            image = cv::Mat(numRows, numCols, CV_32FC1, scn.constPointer(), scn.step());
            image.convertTo(image, CV_8UC1, 255);
        }
    } else if (numChns == 3){
        // RGB VIDEO
        if (numByts == sizeof(unsigned char)){
            image = cv::Mat(numRows, numCols, CV_8UC3, scn.constPointer(), scn.step());
        } else if (numByts == sizeof(unsigned short)){
            image = cv::Mat(numRows, numCols, CV_16UC3, scn.constPointer(), scn.step());
            image.convertTo(image, CV_8UC3);
        } else if (numByts == sizeof(float)){
            image = cv::Mat(numRows, numCols, CV_32FC3, scn.constPointer(), scn.step());
            image.convertTo(image, CV_8UC3);
        }
        cv::cvtColor(image, image, cv::COLOR_RGB2GRAY);
    } else if (numChns == 4){
        // RGBA VIDEO
        if (numByts == sizeof(unsigned char)){
            image = cv::Mat(numRows, numCols, CV_8UC4, scn.constPointer(), scn.step());
        } else if (numByts == sizeof(unsigned short)){
            image = cv::Mat(numRows, numCols, CV_16UC4, scn.constPointer(), scn.step());
            image.convertTo(image, CV_8UC3);
        } else if (numByts == sizeof(float)){
            image = cv::Mat(numRows, numCols, CV_32FC4, scn.constPointer(), scn.step());
            image.convertTo(image, CV_8UC3);
        }
        cv::cvtColor(image, image, cv::COLOR_RGBA2GRAY);
    }
    return(image);
}
