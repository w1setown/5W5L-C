.PHONY: clean All

All:
	@echo "==========Building project:[ 5Words5Letters - Release ]=========="
	@cd "5Words5Letters" && "$(MAKE)" -f  "5Words5Letters.mk"
clean:
	@echo "==========Cleaning project:[ 5Words5Letters - Release ]----------"
	@cd "5Words5Letters" && "$(MAKE)" -f  "5Words5Letters.mk" clean
