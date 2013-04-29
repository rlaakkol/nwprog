##
##all: fork_srv select_srv
##
##fork_srv:
##	$(MAKE) -C forking_server
##	ln -s forking_server/npbsrv fork_srv
##
##select_srv:
##	$(MAKE) -C selecting_server
##	ln -s selecting_server/hdserver select_srv



SUBDIRS = forking_server selecting_server

all: fork_srv select_srv

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

fork_srv: forking_server
	ln -fs forking_server/npbsrv fork_srv

select_srv: selecting_server
	ln -fs selecting_server/hdserver select_srv

