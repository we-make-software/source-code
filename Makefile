BRANCH = main

pull:
	git pull origin $(BRANCH) --rebase

push:
	git add .
	git commit -m "Auto-update: $(shell date)"
	git push origin $(BRANCH)

sync: pull push
