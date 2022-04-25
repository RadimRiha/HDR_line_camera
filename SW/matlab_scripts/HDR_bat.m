%%
cellIm = cell(1,5);
cellIm{1} = imread('bat_hdr/HDR_50.png');
cellIm{2} = imread('bat_hdr/HDR_100.png');
cellIm{3} = imread('bat_hdr/HDR_200.png');
cellIm{4} = imread('bat_hdr/HDR_400.png');
cellIm{5} = imread('bat_hdr/HDR_800.png');
expTimes = [50,100,200,400,800];

%%
cellIm = cell(1,3);
cellIm{1} = imread('bat_hdr/1.png');
cellIm{2} = imread('bat_hdr/2.png');
cellIm{3} = imread('bat_hdr/3.png');
expTimes = [100,200,400];

%%
hdr2 = zeros(2048,2048);
for x = 1:2048
    for y = 1:2048
        numerator = 0;
        denominator = 0;
        for i = 1:3
            pixelValue = cellIm{i}(x,y);
            numerator = numerator + hat(cast(pixelValue,"double")) * (response2(pixelValue) - log(expTimes(i)/10^6));
            denominator = denominator + hat(cast(pixelValue,"double"));
        end
        hdr2(x,y) = numerator / denominator;
    end
end

%%

hdr1 = makehdr(cellIm, 'RelativeExposure', expTimes./expTimes(1));
hdr1 = hdr1(:,:,1);

maxVal = 255;
tonemapped = zeros(2048,2048);

max = max(max(hdr2));
min = min(min(hdr2));
alpha = 0;
tau = alpha * (max-min);

for x = 1:2048
    for y = 1:2048
        tonemapped(x,y) = (log(hdr2(x,y)+tau) - log(min+tau)) / (log(max+tau)-log(min+tau)) * maxVal;
    end
end

figure
imshow([tonemap(hdr1) tonemap(hdr2) tonemapped])
