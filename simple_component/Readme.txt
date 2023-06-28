Simple Component HPX Program:
1. Build-Verzeichnis anlegen
2. Scripts-Verzeichnis anlegen mit der jeweiligen load-env.sh
a. Wichtig ist in load-env, dass "spack load gcc@11.3.0" auch drin steht sonst dicker Fehler weil MPI nicht gefunden wird!!
3. Ins scripts-verzeichnis und ". load-env.sh"
4. Ins build-verzeichnis navigieren
5. Dann "cmake .."
6. "cmake --build ."
7. Ins Scripts-Verzeichnis wechseln
8. "sbatch launch_simple_component"
9. Output ist im scripts-Verzeichnis in "simple_component.txt"

--> bisher nur simple_component_local benchmark ausfŸhrbar, simple_component_remote leider noch nicht






