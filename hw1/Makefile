HOME = /RAID2/COURSE/2025_Spring/mlchip/mlchipTA01
LIB_DIR=$(HOME)/systemc-2.3.3/lib-linux64
INC_DIR=$(HOME)/systemc-2.3.3/include
LIB=-lsystemc-2.3.3
RPATH=-Wl,-rpath,$(HOME)/systemc-2.3.3/lib-linux64

# export SYSTEMC_HOME=/usr/local/systmec-2.3.4
# export LD_LIBRARY_PATH=$(SYSTEMC_HOME)/lib-linux64

O = run
C = *.cpp

cat:
	g++ -I . -I  $(INC_DIR) -L . -L $(LIB_DIR) -o $(O) $(C) $(LIB) $(RPATH) -g
	./run cat.txt
dog:
	g++ -I . -I  $(INC_DIR) -L . -L $(LIB_DIR) -o $(O) $(C) $(LIB) $(RPATH) -g
	./run dog.txt
clean:
	rm -rf $(O)
