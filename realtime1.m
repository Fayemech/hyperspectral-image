clc;
clear all;
close all;



name = 'ledIllumination.lau';
f = imfinfo(name);

global image p;
tif = 'tif';
format = f.Format;
if (strcmp(format ,tif) == 0 )
    disp('wrong input');
end
slice = size(f, 1);
width = f.Width;
height = f.Height;
image = zeros(height, width, slice);

for i = 1 : slice
    image(: , : , i) = imread(name, i);
    %image(:,:,(i-1)*3+1:i*3)=imread(name,i); 
end


p = rowToWavelength(size(image,1));

w = sum(image,1);
w = reshape(w,[size(w,2) size(w,3)])';
subplot(211);
imagesc(w);
set(gcf,'WindowButtonUpFcn',@ButttonUpFcn);



function ButttonUpFcn(src,event)

global image p;
pt = get(gca,'CurrentPoint');
x = pt(1,1);
y = pt(1,2);
x = round(x);
y = round(y);
subplot(212);
plot(p, image(:, x, y));
end