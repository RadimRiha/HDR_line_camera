clear
close all
clc

img = imread('RGB.bmp');

startLine = 1;
R = img(startLine:3:end,:);
G = img(startLine+1:3:end,:);
B = img(startLine+2:3:end,:);

%R = R(1:end-1,:);
%G = G(1:end-1,:);

RGBimg = cat(3, R, G, B);

figure
imshow(R)

figure
imshow(G)

figure
imshow(B)

figure
imshow(RGBimg)

imwrite(R,'RGB_R.bmp')
imwrite(G,'RGB_G.bmp')
imwrite(B,'RGB_B.bmp')
imwrite(RGBimg,'RGB_merge.bmp')

