#!/bin/bash

#script che si occupa di lanciare una nuova sessione tmux
#dividere la finestra in 6 parti e lancia in ognuna di queste
#un'istanza del server e dei 5 peer
tmux new-session -d -y 512 -x 64 "./ds 0" \;\
    split-window "./peer 0" \;\
    split-window "./peer 0" \;\
    split-window "./peer 0" \;\
    split-window "./peer 0" \;\
    split-window "./peer 0" \;\
    select-layout tiled \;\
    attach
