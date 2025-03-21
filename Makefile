BRANCH = main
COMMIT_MSG = Update on $(shell date '+%Y-%m-%d %H:%M:%S')
all:
	$(MAKE) -C ExpiryWorkBase start
	$(MAKE) -C TheMailConditioner start
	$(MAKE) -C TheMaintainer start
	$(MAKE) -C ThePostOffice start

clean:
	$(MAKE) -C ThePostOffice clean	
	$(MAKE) -C TheMaintainer clean
	$(MAKE) -C TheMailConditioner clean
	$(MAKE) -C ExpiryWorkBase clean

stop:
	$(MAKE) -C ThePostOffice stop
	$(MAKE) -C TheMaintainer stop
	$(MAKE) -C TheMailConditioner stop
	$(MAKE) -C ExpiryWorkBase stop
	make clean

log:
	dmesg -w

clear:
	dmesg -C

pull:
	$(MAKE) -C TheMailConditioner pull
	$(MAKE) -C ThePostOffice pull
	$(MAKE) -C TheMaintainer pull
	$(MAKE) -C ExpiryWorkBase pull
	$(MAKE) -C TheRequirements pull
	git pull origin main --rebase

commit:
	$(MAKE) -C ExpiryWorkBase commit
	$(MAKE) -C TheMailConditioner commit
	$(MAKE) -C ThePostOffice commit
	$(MAKE) -C TheMaintainer commit
	git add .
	git commit -m "Updated main repository"
	git push origin main

new:
	git submodule add https://github.com/we-make-software/$(name).git $(name)
	make commit
	echo '[submodule "$(name)"]\n\tpath = $(name)\n\turl = https://github.com/we-make-software/$(name).git' >> .gitmodules

hard:
	git push origin main --force

restart:
	@$(MAKE) clean
	@sudo reboot	