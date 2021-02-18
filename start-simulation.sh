#!/usr/bin/bash

# passa nella cartella della simulazione
cd simulation
# genera i file per la simulazione
make generate-register
for i in {3001..3005}
do
./generate-register $i
done
# avvia la simulazione
tmux new-session -d -y 512 -x 64 "../ds 3000; sleep 5" \;\
    split-window "../peer 3001; sleep 5" \;\
    split-window "../peer 3002; sleep 5" \;\
    split-window "../peer 3003; sleep 5" \;\
    split-window "../peer 3004; sleep 5" \;\
    split-window "../peer 3005; sleep 5" \;\
    select-layout tiled \;\
    attach
