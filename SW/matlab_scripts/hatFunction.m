clear
close all
clc

figure
plot(0:255,[0:127,127:-1:0])
grid on
title('Hat funkce')
xlabel('hodnota pixelu')
ylabel('hodnota váhové funkce')
xlim([0, 255]);