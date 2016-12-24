
MAKE := make --no-print-directory

all :
	@$(MAKE) -f makefile_common clean;
	@$(MAKE) -f makefile_common;

	@$(MAKE) -f makefile_RegistClient clean;
	@$(MAKE) -f makefile_RegistClient;
	
	@$(MAKE) -f makefile_ValidateClient clean;
	@$(MAKE) -f makefile_ValidateClient;
	
	@$(MAKE) -f makefile_RegistServer clean;
	@$(MAKE) -f makefile_RegistServer;
	
	@$(MAKE) -f makefile_ValidateServer clean;
	@$(MAKE) -f makefile_ValidateServer;

clean :
	@$(MAKE) -f makefile_common clean;
	@$(MAKE) -f makefile_RegistClient clean;
	@$(MAKE) -f makefile_ValidateClient clean;	
	@$(MAKE) -f makefile_RegistServer clean;	
	@$(MAKE) -f makefile_ValidateServer clean;
	


