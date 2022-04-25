clear
close all
clc

batScans = ["bat/50us.png","bat/100us.png","bat/150us.png","bat/200us.png","bat/250us.png","bat/300us.png","bat/350us.png","bat/400us.png","bat/500us.png","bat/1000us.png"];
batExpTimes = [50,100,150,200,250,300,350,400,500,1000];

response1 = camresponse(["HDR_2.png","HDR_3.png"],'ExposureTimes',[150,1000]);
response2 = camresponse(batScans,'ExposureTimes',batExpTimes);
response2 = response2(:,1);
range = 0:length(response1)-1;

writematrix(response2','responseFunction.csv')

figure
hold on
%plot(response1,range);
plot(response2,range);
xlabel('Log-Exposure');
ylabel('Image Intensity');
title('Camera Response Function');
grid on
axis('tight')
