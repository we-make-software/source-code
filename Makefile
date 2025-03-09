# Set your branch name
BRANCH = main

# Pull the latest changes from GitHub
pull:
	git pull origin $(BRANCH) --rebase

# Push your local changes to GitHub
push:
	git add .
	git commit -m "Auto-update: $(shell date)"
	git push origin $(BRANCH)

# Full workflow: pull first, then push
sync: pull push
