clear
close all
clc

img = imread('HDR.bmp');

startLine = 1;
E1 = img(startLine:3:end,:);
E2 = img(startLine+1:3:end,:);
E3 = img(startLine+2:3:end,:);

figure
imshow(E1)

figure
imshow(E2)

figure
imshow(E3)

imwrite(E1,'HDR_1.bmp')
imwrite(E2,'HDR_2.bmp')
imwrite(E3,'HDR_3.bmp')

