BRANCH = main
COMMIT_MSG = Update on $(shell date '+%Y-%m-%d %H:%M:%S')
all:
	$(MAKE) -C ExpiryWorkBase start
	$(MAKE) -C TheMailConditioner start

stop:
	$(MAKE) -C TheMailConditioner stop
	$(MAKE) -C ExpiryWorkBase stop

log:
	dmesg -w

clear:
	dmesg -C

pull:
	$(MAKE) -C ThePosTheMailConditioner pull
	$(MAKE) -C ExpiryWorkBase pull
	git pull origin main --rebase

commit:
	git add .
	git commit -m "Updated main repository"
	git push origin main

submodule:
	git submodule add https://github.com/we-make-software/$(name).git $(name)
	make commit


hard:
	git reset --hard origin/main
	git push origin main --force