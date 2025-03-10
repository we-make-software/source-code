savedcmd_ThePostOffice.mod := printf '%s\n'   ThePostOffice.o | awk '!x[$$0]++ { print("./"$$0) }' > ThePostOffice.mod
