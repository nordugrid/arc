# Gnuplot script file for plotting data in file "timings_depth_vs_time_30x5.dat"
# This file is called   force.p
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set title "Time to create and stat collections at different hierarchy levels without security"
set xlabel "Depth"
set ylabel "Time (s)"
# set key 0.01,100
# set label "Yield Point" at 0.003,260
# set arrow from 0.0028,250 to 0.003,280
set xr [0:30]
set yr [0:1.7]
set terminal postscript landscape enhanced mono dashed lw 1 "Helvetica" 14 
set output "timings_depth_vs_time_30x5.ps"
plot    "timings_depth_vs_time_30x5.dat" using 1:6 title 'Creating' with linespoints , \
        "timings_depth_vs_time_30x5.dat" using 1:12 title 'Stating' with points
