# gpstyle classic
set linetype 1 pt 1 lc rgb "#FF0000"
set linetype 2 pt 2 lc rgb "#0000FF"
set linetype 3 pt 3 lc rgb "#00CC00"
set linetype 4 pt 4 lc rgb "#BF25B2"
set linetype 5 pt 5 lc rgb "#8FAAB3"
set linetype 6 pt 6 lc rgb "#FFA500"
set linetype 7 pt 7 lc rgb "#E51E10"
set linetype 8 pt 8 lc rgb "#000000"
set key left top
set xlabel "h"
# start literal lines
set xlabel 'forecast horizon'
set title 'Forecasts of exp(LRY)'
set xtics 1
# end literal lines
# start inline data
$data << EOD
1 420.5880879 423.6932454 423.6968311 423.3723111 423.703058 18.57854761 425.9497001 13.67942539
2 423.0115781 422.8335783 422.8371567 424.2319274 428.9540274 28.28183097 424.919959 18.87089054
3 421.6669762 422.0171994 422.0207708 423.3311504 424.6762383 30.77033519 424.5393065 21.16217281
4 421.438698 421.2418543 421.2454192 422.3056173 426.3828439 40.93096615 422.854784 24.70028653
5 420.6796713 420.5054133 420.508972 421.571302 423.9877797 41.49408104 422.2671626 26.69469662
6 419.5872082 419.8058636 419.8094163 420.8426369 426.1550572 45.8287151 421.9009317 29.97945561
EOD
# end inline data
plot \
$data using 1:($6-$7):($6+$7) notitle lc rgb "#e5e5e5" w filledcurve, \
$data using 1:($8-$9):($8+$9) notitle lc rgb "#cccccc" w filledcurve, \
$data using 1:2 title 'q50' w lines lt 1, \
$data using 1:3 title 'efc_gen' w lines lt 2, \
$data using 1:4 title 'efc_norm' w lines lt 3, \
$data using 1:5 title 'efc_mean' w lines lt 4
