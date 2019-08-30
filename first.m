clc
clear all;
close all;

name = '400-430-500-600-700.lau';
f = imfinfo(name);

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

cie = xlsread('cie.xlsx');
cie_r = cie(:,1);
cie_r(:,2) = cie(:,2);
cie_g = cie(:,1);
cie_g(:,2) = cie(:,3);
cie_b = cie(:,1);
cie_b(:,2) = cie(:,4);

p = rowToWavelength(size(image,1));

x_r = cie_r(:,1);
y_r = cie_r(:,2);
i_r = interpice1(x_r, y_r, p);

x_g = cie_g(:,1);
y_g = cie_g(:,2);
i_g = interpice1(x_g, y_g, p);

x_b = cie_b(:,1);
y_b = cie_b(:,2);
i_b = interpice1(x_b, y_b, p);

w = sum(image,1);
w = reshape(w,[size(w,2) size(w,3)])';


[row, col] = size(w);

g = image(:,805, 201);
l = find(g > 0.002);

red = zeros(size(w));
blue = zeros(size(w));
green = zeros(size(w));

for r = 1 : size(image,3)
    for c = 1 : size(image,2)
        pix = image(:,c,r);
        red(r,c) = sum(pix(l) .* i_r(l) ./ g(l));           
        green(r,c) = sum(pix(l) .* i_g(l) ./ g(l));
        blue(r,c) = sum(pix(l) .* i_b(l) ./ g(l));
    end
end
rgb = imresize(cat(3, red, green, blue)/1.2, [480, 640]);
imshow(rgb)



%for i=1:slice
%    J=uint8(image(:,:,i));                                   %%
%    %%imwrite(J,[num2str(i,'%4d'),'.tif'],'WriteMode','Append');
%    imwrite(J,[num2str(i,'%04d'),'.tif']);
%end



   

