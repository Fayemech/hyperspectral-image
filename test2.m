function varargout = test2(varargin)
% TEST2 MATLAB code for test2.fig
%      TEST2, by itself, creates a new TEST2 or raises the existing
%      singleton*.
%
%      H = TEST2 returns the handle to a new TEST2 or the handle to
%      the existing singleton*.
%
%      TEST2('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in TEST2.M with the given input arguments.
%
%      TEST2('Property','Value',...) creates a new TEST2 or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before test2_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to test2_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help test2

% Last Modified by GUIDE v2.5 03-Sep-2019 16:01:16

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @test2_OpeningFcn, ...
                   'gui_OutputFcn',  @test2_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before test2 is made visible.
function test2_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to test2 (see VARARGIN)

% Choose default command line output for test2
handles.output = hObject;
% Update handles structure
guidata(hObject, handles);


% UIWAIT makes test2 wait for user response (see UIRESUME)
% uiwait(handles.figure1);



% --- Outputs from this function are returned to the command line.
function varargout = test2_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes on button press in pushbutton1.



% --------------------------------------------------------------------
function Untitled_1_Callback(hObject, eventdata, handles)
% hObject    handle to Untitled_1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)



% --------------------------------------------------------------------
function open_Callback(hObject, eventdata, handles)
% hObject    handle to open (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

[filename, pathname] = uigetfile('*.lau');
str=[pathname filename];
if isequal(filename,0)||isequal(pathname,0)
    warndlg('Please select a picture first!','Warning');
    return;
else
    name = str;
    f = imfinfo(name);

    global target p;
    slice = size(f, 1);
    width = f.Width;
    height = f.Height;
    target = zeros(height, width, slice);

    for i = 1 : slice
        
        target(: , : , i) = imread(name, i);
        %image(:,:,(i-1)*3+1:i*3)=imread(name,i); 
    end


    p = rowToWavelength(size(target,1));

 
    cie = xlsread('cie.xlsx');
    cie_r = cie(:,1);
    cie_r(:,2) = cie(:,2);
    cie_g = cie(:,1);
    cie_g(:,2) = cie(:,3);
    cie_b = cie(:,1);
    cie_b(:,2) = cie(:,4);
    x_r = cie_r(:,1);
    y_r = cie_r(:,2);
    i_r = interpice1(x_r, y_r, p);

    x_g = cie_g(:,1);
    y_g = cie_g(:,2);
    i_g = interpice1(x_g, y_g, p);

    x_b = cie_b(:,1);
    y_b = cie_b(:,2);
    i_b = interpice1(x_b, y_b, p);
    w = sum(target,1);
    w = reshape(w,[size(w,2) size(w,3)])';
    red = zeros(size(w));
    blue = zeros(size(w));
    green = zeros(size(w));


    for r = 1 : size(target,3)
        for c = 1 : size(target,2)
            pix = target(:,c,r);
            red(r,c) = sum(pix .* i_r);           
            green(r,c) = sum(pix .* i_g);
            blue(r,c) = sum(pix .* i_b);
        end
    end
    
    global rgb h1 h2 ROIS;
    rgb = cat(3, red, green, blue);
    lm = max(max(rgb));
    rgb(:,:,1) = rgb(:,:,1) / lm(1);
    rgb(:,:,2) = rgb(:,:,2) / lm(2);
    rgb(:,:,3) = rgb(:,:,3) / lm(3);
    h1=axes('Position',[0.05 0.55 0.9 0.45]);
    h2=axes('Position',[0.05 0.05 0.9 0.45]);
    axes(h1);
    imshow(rgb);
    global spec;
    spec = 0;
    set(gca, 'DataAspectRatioMode','auto');
    
end

set(gcf,'WindowButtonUpFcn',@ButttonUpFcn);
set(gcf,'WindowKeyPressFcn',@WindowKeyPressFcn);

    


% --------------------------------------------------------------------


% --------------------------------------------------------------------
function exit_Callback(hObject, eventdata, handles)
% hObject    handle to exit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)




function pushbutton1_Callback(hObject, eventdata, handles)
% hObject    handle to pushbutton1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)


function WindowKeyPressFcn(src, event, handles)
i = event.Key;
global rgb p target P h2 ROIS spec;
if i == 'w'
    figure(3);
    set(gcf, 'position', [500 200 1000 1000]);
    imshow(rgb);
    set(gca, 'DataAspectRatioMode','auto');
    [b rect] = imcrop();
    x_axes = round(rect(1));
    y_axes = round(rect(2));
    wth = round(rect(3));
    hgt = round(rect(4));
    %index = 1;
    %spec = zeros(size(target, 1), 1);
    for r = x_axes : 1 : (x_axes + wth)
        for c = y_axes : 1 : (y_axes + hgt)
            %spec(index, :) = target(:, r, c);
            spec = spec + target(:, r, c);
        end
    end
    %spec = sum(spec, 1);
    spec = spec / (size(b, 1) * size(b, 2));
    close(figure(3));
    axes(h2);
    cla;
    if (~isempty(ROIS))
        plot(p, ROIS);
    end
    plot(p, spec);
    hold on;
    
elseif i == 'q'
    P = [];
    figure(2);
    imshow(rgb);
    set(gca, 'DataAspectRatioMode','auto');
    a = roipoly();
    sie = max(size(target, 1)) + 2;
    P = zeros(sum(a(:)), sie);
    ind = 1;
    for r = 1:size(a,1)
        for c = 1:size(a,2)
            if (a(r,c))
                P(ind, 1) = r;
                P(ind, 2) = c;
                P(ind, 3 : sie) = target(:, c, r);
                ind = ind + 1;
            end
        end
    end
    global save;
    save = [];
    save = P;
    ind = ind - 1;
    ori = sum(P);
    ori = ori / ind;
    ori = ori(3 : end);
    
    e = ones(size(P, 2), 1)';
    for i = 1 : size(P, 2)
        cols = P(:, i);
        e(i) = e(i) * std(cols);
    end
    e = e(3: end);
    for j = 1 : max(size(spec))
        if spec(j) < 0.01
            P(:, j) = 0;
            e(j) = 0;
        else
            P(:, j) = P(:, j) ./ spec(j);
        end
    end
    sp = sum(P);
    sp = sp / ind;
    sp = sp(3 : end);
    ROIS = [ROIS; sp];
    close(figure(2))
    %sp = sp ./ max(max(sp));
    %for j = 1 : max(size(sp))
    %    if spec(j) < 0.1
    %       sp(j) = 0;
    %    else
    %       sp(j) = sp(j) ./ spec(j);
    %    end
    %end
    axes(h2);
    %e = std(sp)*ones(size(p));
    if size(ROIS, 1) == 1
        shadedErrorBar(p,sp,e, 'lineprops', '-r');
        plot(p, ori, 'r');
    elseif size(ROIS, 1) == 2
        shadedErrorBar(p,sp,e, 'lineprops', '-g');
        plot(p, ori, 'g');
    elseif size(ROIS, 1) == 3
        shadedErrorBar(p,sp,e, 'lineprops', '-b');
        plot(p, ori, 'b');
    elseif size(ROIS, 1) == 4
        shadedErrorBar(p,sp,e, 'lineprops', '-y');
        plot(p, ori, 'y');
    elseif size(ROIS, 1) == 5
        shadedErrorBar(p,sp,e, 'lineprops', '-c');
        plot(p, ori, 'c');
    else
        shadedErrorBar(p,sp,e, 'lineprops', '-r');
        plot(p, ori, 'r');
    end
    
    %plot(p, sp);
    hold on;
    
    %csvwrite('sample0501.csv', P);
elseif i == 'e'
    ROIS = [];
    hold off;
    cla;
    if (~isempty(spec))
        axes(h2);
        plot(p, spec);
        hold on;
    end
else
    warndlg('press a right key.','Warning');
    %disp('press a right key');
end

function save_Callback(hObject, eventdata, handles)
% hObject    handle to save (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global save spec;
c = [-1, -1];
c = [c  spec'];
c = [c ; save];
[file,path] = uiputfile('*.tif');
if file == 0
    return;
else
   str=[path file];
   t = Tiff(str,'w');
   tagstruct.ImageLength = size(c,1);
   tagstruct.ImageWidth = size(c,2);
   tagstruct.Photometric = Tiff.Photometric.MinIsWhite;
   tagstruct.Compression = Tiff.Compression.LZW;
   tagstruct.SampleFormat = 3;
   tagstruct.BitsPerSample = 64;
   tagstruct.SamplesPerPixel = 1;
   tagstruct.RowsPerStrip = 16;
   tagstruct.PlanarConfiguration = Tiff.PlanarConfiguration.Chunky;
   tagstruct.Software = 'MATLAB';
   t.setTag(tagstruct);
   t.write(c);
   t.close;
end

function ButttonUpFcn(src, event, handles)

global target p h2;

pt = get(gca,'CurrentPoint');
x = pt(1,1);
y = pt(1,2);
x = round(x);
y = round(y);
axes(h2);
plot(p, target(:, x, y));
set(gca, 'DataAspectRatioMode','auto');
