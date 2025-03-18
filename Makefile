BRANCH = main
COMMIT_MSG = Update on $(shell date '+%Y-%m-%d %H:%M:%S')
all:
	$(MAKE) -C ExpiryWorkBase start
	$(MAKE) -C ThePostOffice start

stop:
	$(MAKE) -C ThePostOffice stop
	$(MAKE) -C ExpiryWorkBase stop

log:
	dmesg -w

clear:
	dmesg -C

pull:
	$(MAKE) -C ThePostOffice pull
	$(MAKE) -C ExpiryWorkBase pull
	git pull origin main --rebase

commit:
	$(MAKE) -C ThePostOffice commit
	$(MAKE) -C ExpiryWorkBase commit
	@if ! git diff-index --quiet HEAD; then \
		git add . && \
		git commit -m "$(COMMIT_MSG)" && \
		git push origin main; \
	else \
		echo "No changes in $(PWD) to commit."; \
	fi

#git submodule add https://github.com/we-make-software/TheCheckPoint.git TheCheckPoint

