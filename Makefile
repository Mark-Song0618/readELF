test: test.cc parseElf.cc parseElf.hh
	g++ -g test.cc parseElf.cc -o $@ 

.PHONY: clean
clean:
	rm ./test *.o -rf
