.PHONY: clean All

All:
	@echo "==========Building project:[ prototype - Debug ]=========="
	@cd "prototype" && "$(MAKE)" -f  "prototype.mk"
clean:
	@echo "==========Cleaning project:[ prototype - Debug ]----------"
	@cd "prototype" && "$(MAKE)" -f  "prototype.mk" clean
