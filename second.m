clc;
clear all;
close all;
%%name should be tiff file name and barname should be cie1931 chart
name = 'ledIllumination.lau';
barname = 'cie.xlsx';

rgb = HsiToRgb(name, barname);

imshow(rgb);