clear;
if 1
f=fopen(['20220601.f50.f103'],'r');
x=fread(f,[103,inf],'float32');
else
f=fopen(['20211024ASK.f48.f100'],'r');
x=fread(f,[100,inf],'float32');
end
fclose(f);

time=linspace(0,length(x)/100/60,length(x));

if 0
delta=x(98,:)-x(84,:);
delta( delta > pi) = delta( delta > pi) - 2*pi;
delta( delta < -pi) = delta( delta < -pi) + 2*pi;
plot(time, delta*180/pi)
end
