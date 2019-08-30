function rgb = HsiToRgb(TIFFname, barname)

%
% first input name should be TIFF file;
% second input barname should the CIE1931 file with wavelength be the first
% col, red bar second col, green bar third col, blue bar fourth col.
%
%


f = imfinfo(TIFFname);

%% check if the input tiff file right
tif = 'tif';
format = f.Format;
if (strcmp(format ,tif) == 0 )
    disp('wrong input');
end

%% read tiff to data cube target
slice = size(f, 1);
width = f.Width;
height = f.Height;

target = zeros(height, width, slice);

for i = 1 : slice
    target(: , : , i) = imread(TIFFname, i);
    %image(:,:,(i-1)*3+1:i*3)=imread(name,i); 
end

%% interpolate the bar value
cie = xlsread(barname);
cie_r = cie(:,1);
cie_r(:,2) = cie(:,2);
cie_g = cie(:,1);
cie_g(:,2) = cie(:,3);
cie_b = cie(:,1);
cie_b(:,2) = cie(:,4);

p = rowToWavelength(size(target,1));

x_r = cie_r(:,1);
y_r = cie_r(:,2);
i_r = interpice1(x_r, y_r, p);

x_g = cie_g(:,1);
y_g = cie_g(:,2);
i_g = interpice1(x_g, y_g, p);

x_b = cie_b(:,1);
y_b = cie_b(:,2);
i_b = interpice1(x_b, y_b, p);

%% sum the first axis
w = sum(target,1);
w = reshape(w,[size(w,2) size(w,3)])';

%% pick the brightest point 
g = target(:,805, 201); %bright point;
l = find(g > 0.002);

%% RGB part
red = zeros(size(w));
blue = zeros(size(w));
green = zeros(size(w));


for r = 1 : size(target,3)
    for c = 1 : size(target,2)
        pix = target(:,c,r);
        red(r,c) = sum(pix(l) .* i_r(l) ./ g(l));           
        green(r,c) = sum(pix(l) .* i_g(l) ./ g(l));
        blue(r,c) = sum(pix(l) .* i_b(l) ./ g(l));
    end
end
rgb = imresize(cat(3, red, green, blue)/1.2, [480, 640]);

return