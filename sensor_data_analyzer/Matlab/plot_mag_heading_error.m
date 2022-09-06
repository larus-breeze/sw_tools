time=linspace(0,length(x)/100/60,length(x));

start = 10 * 6000;
[B,A]=butter(2, 0.00001);
delta=x(98,start:length(x))-x(84,start:length(x));
delta( delta > pi) = delta( delta > pi) - 2*pi;
delta( delta < -pi) = delta( delta < -pi) + 2*pi;
plot(time(start:length(x)), delta*180/pi)
grid
hold
plot(time(start:length(x)), filtfilt( B,A, delta*180/pi))
xlabel('Time / min');
ylabel('Angle / Degrees');
title("Heading Error D-GNSS / Magnetic Compass")