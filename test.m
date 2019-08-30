load ref4_scene4.mat;
size(reflectances);
load illum_6500.mat;
radiances_6500 = zeros(size(reflectances)); % initialize array
for i = 1:33,
  radiances_6500(:,:,i) = reflectances(:,:,i)*illum_6500(i);
end
a = radiances_6500;
[r c w] = size(a);
a = reshape(a, r*c, w);

load xyzbar.mat;
XYZ = (xyzbar'*a')';