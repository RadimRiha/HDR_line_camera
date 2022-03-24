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

maxVal = 0xff;
b = 0.85;
Ldmax = 100;
Lwmax = max(max(hdr1));
tonemapped = zeros(2048,2048);
gamma = 2.2;

max = max(max(hdr2));
min = min(min(hdr2));

for x = 1:2048
    for y = 1:2048
        %tonemapped(x,y) = 256 * Ldmax * 0.01 * log(hdr1(x,y)+1) / (log10(Lwmax+1) * log(2+8*((hdr1(x,y)/Lwmax)^(log(b)/log(0.5)))));
        tonemapped(x,y) = (hdr2(x,y) - min) / (max-min) * 256;
    end
end

figure(1)
imshow([tonemap(hdr1) tonemap(hdr2) tonemapped])
