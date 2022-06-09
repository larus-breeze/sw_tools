plot(x(103,:))
plot(time, x(103,:)*180/pi)
grid
ylabel('Angle difference GNSS vs INS / degrees');
xlabel('Time / min.');
title('Heading Angel compared');
hold
delta=x(98,:)-x(84,:);
delta( delta > pi) = delta( delta > pi) - 2*pi;
delta( delta < -pi) = delta( delta < -pi) + 2*pi;
plot(time, delta*180/pi)
legend('SAT minus INS','MAG-AHRS minus SAT-AHRS');