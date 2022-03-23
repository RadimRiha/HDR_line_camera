clear
close all
clc

img = imread('HDR.bmp');

startLine = 1;
E1 = img(startLine:3:end,:);
E2 = img(startLine+1:3:end,:);
E3 = img(startLine+2:3:end,:);

imwrite(E1,'HDR_1.png')
imwrite(E2,'HDR_2.png')
imwrite(E3,'HDR_3.png')

%% hdr generation

cellIm = cell(1,3);
cellIm{1} = E1;
cellIm{2} = E2;
cellIm{3} = E3;

expTimes = [10 150 1000];
hdr = makehdr(cellIm, 'RelativeExposure', expTimes./expTimes(1)); 

figure
imshow([E1 E2 E3 tonemap(hdr)])

