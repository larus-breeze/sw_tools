a=100*6000; 
e=length(x);

gnd_N = 3.6 * resample( x(30,a:e), 2, 100); % GNSS groundspeed
gnd_E = 3.6 * resample( x(31,a:e), 2, 100);

air_N = 3.6 * resample( x(52,a:e) .* cos(x(84,a:e)), 2, 100); % airspeed
air_E = 3.6 * resample( x(52,a:e) .* sin(x(84,a:e)), 2, 100);

wind_N = 3.6 * resample( x(59,a:e), 2, 100); % reported wind fast
wind_E = 3.6 * resample( x(60,a:e), 2, 100);

wind_N_avg = 3.6 * resample( x(62,a:e), 2, 100); % reported wind avg
wind_E_avg = 3.6 * resample( x(63,a:e), 2, 100);

for i=1:length(gnd_N)

%plot wind vector from center
    plot( [0 wind_E_avg(i)], [0 wind_N_avg(i)],'LineWidth',2.0,'Color','blue');
    hold on
    grid
    axis( [ -200 200 -200 200]);
    plot( wind_E(i), wind_N(i),'+');
    
%plot ground vector
    plot( [0 gnd_E(i)], [0 gnd_N(i)],'LineWidth',2.0,'Color','red');

%plot air vector    
    plot( [0 air_E(i) ], [0 air_N(i)],'LineWidth',2.0,'Color','green');

    title('Blue: Mean Wind, Red: Groundspeed, Green: Airspeed, + Inst. Wind');
    xlabel( 'Speed E / km/h');
    ylabel( 'Speed N / km/h');
    pause(0.01);
    hold off;
end
