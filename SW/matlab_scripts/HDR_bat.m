
cellIm = cell(1,5);
cellIm{1} = imread('bat_hdr/HDR_50.png');
cellIm{2} = imread('bat_hdr/HDR_100.png');
cellIm{3} = imread('bat_hdr/HDR_200.png');
cellIm{4} = imread('bat_hdr/HDR_400.png');
cellIm{5} = imread('bat_hdr/HDR_800.png');

expTimes = [50,100,200,400,800];

hdr1 = makehdr(cellIm, 'RelativeExposure', expTimes./expTimes(1));

figure(1)
imshow(tonemap(hdr1))
