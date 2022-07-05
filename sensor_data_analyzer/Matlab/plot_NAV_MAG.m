o = 0;
plot(time, x(69+o,:))
grid
hold
plot(time, x(70+o,:))
plot(time, x(71+o,:))
plot(time, sqrt( x(69+o,:).^2+x(70+o,:).^2+x(71+o,:).^2))
legend('North','East','Down','Magnitude');
xlabel('Time / min.');
ylabel('Mag. Induction norm.');
