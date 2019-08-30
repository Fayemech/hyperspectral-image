function w = rowToWavelength(N)
%
% N = size of first dimension in hyerspectral data cube
% w = wavelength of each element
%

x = [1:N]';

x1 = 375;
x2 = 424;

y1 = 400;
y2 = 500;

A = [x1 1; x2 1];
v = inv(A) * [y1; y2];

w = [x(:) ones(size(x(:)))] * v;

return;
