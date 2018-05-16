#!/bin/sh
if [ "$#" -ne 8 ]; then
    # AQUI PUEDE CAMBIAR
    echo "run as ./benchmark-martin.sh BINARY STARTN ENDN DN STARTN2 ENDN2 DN2 SAMPLES"
    exit;
fi
# AQUI PUEDE CAMBIAR
BINARY=$1
STARTN=$2
ENDN=$3
DN=$4
STARTN2=$5
ENDN2=$6
DN2=$7
SAMPLES=$8
for A in 1 2;
do
    for  N in `seq ${STARTN2} ${DN2} ${ENDN2}`;
    do
        for NT in 1 2 3 4;
        do
            for B in 1 2 3;
            do

                M=0
                S=0
                # AQUI PUEDE CAMBIAR
                #echo "./${BINARY} -t 1 -api $A -m -n ${N} -nt $NT -benchmark $B -time 180"
                for k in `seq 1 ${SAMPLES}`;
                do
                    # AQUI PUEDE CAMBIAR
                    x=`./${BINARY} -t 2 -api $A -m -n ${N} -nt $NT -benchmark $B -time 120`
                    oldM=$M;
                    M=$(echo "scale=10;  $M+($x-$M)/$k"           | bc)
                    S=$(echo "scale=10;  $S+($x-$M)*($x-${oldM})" | bc)
                done
                echo "done"
                MEAN=$M
                VAR=$(echo "scale=10; $S/(${SAMPLES}-1.0)"  | bc)
                STDEV=$(echo "scale=10; sqrt(${VAR})"       | bc)
                STERR=$(echo "scale=10; ${STDEV}/sqrt(${SAMPLES})" | bc)
                TMEAN=${MEAN}
                TVAR=${VAR}
                TSTDEV=${STDEV}
                TSTERR=${STERR}
                # AQUI PUEDE CAMBIAR
                echo "---> N=${N}  NT=${NT}  --> (MEAN, VAR, STDEV, STERR) -> (${TMEAN}[ms], ${TVAR}, ${TSTDEV}, ${TSTERR})"
                # AQUI PUEDE CAMBIAR
                echo "${N}   ${NT}   ${TMEAN}           ${TVAR}           ${TSTDEV} ${TSTERR}" >> data/running-times_T2_A${A}_N${N}_NT${NT}_B${B}.dat
                echo " "
            done
        done
    done
done 
