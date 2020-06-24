all:
	sudo sh ./clean.sh
	sudo sh ./compile.sh

usr=user_program
sample=sample_input_
sample1=$(sample)1/$(target)
sample2=$(sample)2/target_file
target=target_file_
master:
	sudo ./$(usr)/master 1 $(sample1)2 fcntl
	@echo
	sudo ./$(usr)/master 1 $(sample1)2 fcntl
	@echo
	sudo ./$(usr)/master 1 $(sample1)2 mmap
	@echo
	sudo ./$(usr)/master 1 $(sample1)2 mmap
	@echo
	sudo ./$(usr)/master 10 $(sample1)1 $(sample1)2 $(sample1)3 $(sample1)4 $(sample1)5 $(sample1)6 $(sample1)7 $(sample1)8 $(sample1)9 $(sample1)10 fcntl
	@echo
	sudo ./$(usr)/master 10 $(sample1)1 $(sample1)2 $(sample1)3 $(sample1)4 $(sample1)5 $(sample1)6 $(sample1)7 $(sample1)8 $(sample1)9 $(sample1)10 fcntl
	@echo
	sudo ./$(usr)/master 10 $(sample1)1 $(sample1)2 $(sample1)3 $(sample1)4 $(sample1)5 $(sample1)6 $(sample1)7 $(sample1)8 $(sample1)9 $(sample1)10 mmap
	@echo
	sudo ./$(usr)/master 10 $(sample1)1 $(sample1)2 $(sample1)3 $(sample1)4 $(sample1)5 $(sample1)6 $(sample1)7 $(sample1)8 $(sample1)9 $(sample1)10 mmap

ip=127.0.0.1
slave:
	sudo rm -rf output
	mkdir output
	sudo ./$(usr)/slave 1 output/file.out fcntl $(ip)
	sudo diff output/file.out $(sample1)2
	@echo
	sudo ./$(usr)/slave 1 output/file.out mmap $(ip)
	sudo diff output/file.out $(sample1)2
	@echo
	sudo ./$(usr)/slave 1 output/file.out fcntl $(ip)
	sudo diff output/file.out $(sample1)2
	@echo
	sudo ./$(usr)/slave 1 output/file.out mmap $(ip)
	sudo diff output/file.out $(sample1)2
	@echo
	sudo ./$(usr)/slave 10 output/file1.out output/file2.out output/file3.out output/file4.out output/file5.out output/file6.out output/file7.out output/file8.out output/file9.out output/file10.out fcntl $(ip)
	make mult_cmp
	@echo
	sudo ./$(usr)/slave 10 output/file1.out output/file2.out output/file3.out output/file4.out output/file5.out output/file6.out output/file7.out output/file8.out output/file9.out output/file10.out mmap $(ip)
	make mult_cmp
	@echo
	sudo ./$(usr)/slave 10 output/file1.out output/file2.out output/file3.out output/file4.out output/file5.out output/file6.out output/file7.out output/file8.out output/file9.out output/file10.out fcntl $(ip)
	make mult_cmp
	@echo
	sudo ./$(usr)/slave 10 output/file1.out output/file2.out output/file3.out output/file4.out output/file5.out output/file6.out output/file7.out output/file8.out output/file9.out output/file10.out mmap $(ip)
	make mult_cmp

mult_cmp:
	sudo diff output/file1.out $(sample1)1
	sudo diff output/file2.out $(sample1)2
	sudo diff output/file3.out $(sample1)3
	sudo diff output/file4.out $(sample1)4
	sudo diff output/file5.out $(sample1)5
	sudo diff output/file6.out $(sample1)6
	sudo diff output/file7.out $(sample1)7
	sudo diff output/file8.out $(sample1)8
	sudo diff output/file9.out $(sample1)9
	sudo diff output/file10.out $(sample1)10

small_cmp:
	sudo diff output/file.out $(sample1)2

large_cmp:
	sudo diff output/file.out $(sample2)

file=Makefile
upload_file:
	scp $(file) b07902115@linux15.csie.org:~/Documents/OS_Project2/

dest=./
update_file:
	scp b07902115@linux15.csie.org:~/Documents/OS_Project2/$(file) $(dest)