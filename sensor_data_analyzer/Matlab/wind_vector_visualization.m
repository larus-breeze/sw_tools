a=1; e=length(x);

ende = length(x);
t=linspace(0,ende/100/60,ende);

gnd_N = 3.6 *resample( x(30,a:e), 2, 100); % GNSS groundspeed
gnd_E = 3.6 *resample( x(31,a:e), 2, 100);

wind_N = 3.6 * resample( x(59,a:e), 2, 100); % reported wind fast
wind_E = 3.6 * resample( x(60,a:e), 2, 100);

wind_N_avg = 3.6 * resample( x(62,a:e), 2, 100); % reported wind avg
wind_E_avg = 3.6 * resample( x(63,a:e), 2, 100);

wind_delta_N = wind_N - wind_N_avg;
wind_delta_E = wind_E - wind_E_avg;

for i=400:length(gnd_N)
%    hold on;
%   plot( [0; 0], [ wind_E_r(i); wind_N_r(i) ]);
    plot( [0 wind_E_avg(i)], [0 wind_N_avg(i)],'LineWidth',2.0);
    hold on
    grid
    axis( [ -200 200 -200 200]);
    plot( wind_E_avg(i), wind_N_avg(i),'o');
    
    plot( wind_E(i), wind_N(i),'+');
    
    plot( [0 gnd_E(i)], [0 gnd_N(i)],'LineWidth',2.0);
    plot( gnd_E(i), gnd_N(i),'*');

    plot( [0 gnd_E(i) - wind_E_avg(i)], [0 gnd_N(i)- wind_N_avg(i)],'LineWidth',2.0);
 %   plot( [0 gnd_E(i) - wind_E_r(i)], [0 gnd_N(i)- wind_N_r(i)],'LineWidth',2.0);
    plot( gnd_E(i) - wind_E(i), gnd_N(i) - wind_N(i),'X');
 %   plot( 10*wind_delta_E(i), 10*wind_delta_N(i),'*');

    title('O Wind smart AVG   + Wind 1s   x Airspeed meas.  Blue: Airspeed estimate  Purple: Groundspeed');
    xlabel( 'Speed E / km/h');
    ylabel( 'Speed N / km/h');
    pause(0.01);
    hold off;
end
