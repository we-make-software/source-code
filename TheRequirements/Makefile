
COMMIT_MSG = Update on $(shell date '+%Y-%m-%d %H:%M:%S')


commit:
	@if ! git diff-index --quiet HEAD; then \
		git add . && \
		git commit -m "$(COMMIT_MSG)" && \
		git push origin main; \
	else \
		echo "No changes in $(PWD) to commit."; \
	fi

pull:
	git pull origin main --rebase
