a=subplot(2,1,1)
plot(time,x(66,:))
grid
hold
plot(time,x(33,:))
legend( 'INS Acceleration','GNSS Acceleration');
xlabel('Time / min.');
ylabel('Acceleration m/s^2');
title('Acceleration North');

b=subplot(2,1,2)
plot(time,x(67,:))
grid
hold
plot(time,x(34,:))
legend( 'INS Acceleration','GNSS Acceleration');
xlabel('Time / min.');
ylabel('Acceleration m/s^2');
title('Acceleration East');

linkaxes([a b],'x');

