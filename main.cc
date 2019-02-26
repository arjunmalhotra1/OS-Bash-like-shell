#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

void parse_and_run_command_pipe(std::vector<std::string> commandvector)
{
		int outputrediflag=0,inputrediflag=0;;
		int outputredi=-1,inputredi=-1;
		int count_outputredi=0,count_inputredi=0;
		int count_pipes=0;
		std::vector <int> pipe_index;


		int index_oflist;

		for(int i=0;i<commandvector.size();i++)
		{
			if(commandvector[i].compare("|")==0)
			{
				if(i!=commandvector.size()-1 && commandvector[i+1].compare("|")==0)
				{
					std::cerr<<"Invalid command. 3"<<std::endl;
					return;
				}
				pipe_index.push_back(i);
				count_pipes++;
			}
		}

		if(commandvector[commandvector.size()-1].compare("|")==0)
		{
			std::cerr<<"Invalid command. 4"<<std::endl;
			return;
		}

		std::string tocheckexit=commandvector[0];
		if (tocheckexit.compare("exit")==0)
		{
			exit(0);
		}

		int num_of_commands=count_pipes+1;
		int status[num_of_commands]={0};
		int pipefileds[2*count_pipes]={0};

		int start_index=0;
		int end_index;
		int inputflag=1,outputflag=1;
		int input_redi_index=-1,output_redi_index=-1;

		for(int g=1;g<=num_of_commands;g++)
		{
			if(g==num_of_commands)
			{
				end_index=commandvector.size();
			}
			else
			{
				end_index=pipe_index[g-1];
			}
			for(int y=start_index;y<end_index;y++)
			{
				if(g!=1 && commandvector[y].compare("<")==0)
				{
					//if g is not the first command then and we get input redirection then we should exit
					std::cerr<<"Invalid command. 5"<<std::endl;
					return;
				}

				if(g!=num_of_commands && commandvector[y].compare(">")==0)
				{
					std::cerr<<"Invalid command. 6"<<std::endl;
					return;
					//If g is not the last command then if we get output redirection then exit
				}

				if(commandvector[y].compare("<")==0)
				{
					if(inputflag==0 || (y!=commandvector.size()-1 && commandvector[y+1].compare("|")==0))
					{
						std::cerr<<"Invalid command. 7"<<std::endl;
						return;
					}

					inputflag=0;
					input_redi_index=y;
				}

				if(commandvector[y].compare(">")==0)
				{
					if(outputflag==0 || y==commandvector.size()-1)
					//since I am checking that ">" will come in the last command,
					//I don't have to check for, if there would be a pipe after ">"
					{
						std::cerr<<"Invalid command. 8"<<std::endl;
						return;
					}
					outputflag=0;
					//store the index of output flag.
					output_redi_index=y;
				}
				start_index=pipe_index[g-1]+1;

			}
		}

		for(int i=0;i<2*count_pipes;i=i+2)
		{
			if(pipe(pipefileds+i)<0)
			{
				perror("Pipes couldn't be opened initial open");
				return;

			}
		}
		int child_id_array[num_of_commands]={0};
		//This array stores all the child ids created, so that parent an look at them and wait for them to close.
		int start_index_for_list=0;
		int end_index_for_list;

		for(int j=1;j<=num_of_commands;j++)
		{
			char*  list[commandvector.size()+1];
			int index_passed_tolist=0;

			if(j==num_of_commands)
			{
				end_index_for_list=commandvector.size();
			}
			else
			{
				end_index_for_list=pipe_index[j-1];
			}
			for(int y=start_index_for_list;y<end_index_for_list;y++)
			{

				if((commandvector[y].compare("<")==0) || commandvector[y].compare(">")==0)
				{
					y++;
					continue;
				}

				list[index_passed_tolist]=(&commandvector[y][0]);
				index_passed_tolist++;

			}
				//handle the case when multiple
			start_index_for_list=pipe_index[j-1]+1;


			list[index_passed_tolist]=(char *) 0;


			if(index_passed_tolist==0) //TO check if command is null.
			{
				std::cerr<<"Invalid command."<<std::endl;
				return;
			}

			int childid=fork();
			child_id_array[j-1]=childid;
			if(childid < 0)
			{
				perror("Child could not be created, Please try later !");
				return;
			}
			else if(childid==0)
			{
				if(j==num_of_commands && outputflag==0)
				{
					int fd_out=open(&commandvector[output_redi_index+1][0],O_CREAT | O_WRONLY | O_TRUNC,0644);
					if(fd_out<0)
					{
						perror("Invalid command. fd_out");
						exit(1);
					}

					if(dup2(fd_out,STDOUT_FILENO)<0)
					{
						perror("Invalid command. dup2");
						exit(1);
					}
					close(fd_out);

				}

				if(j==1 && inputflag==0)
				{
					int fd_in=open(&commandvector[input_redi_index+1][0],O_RDONLY,0644);
					if(fd_in<0)
					{
						perror("Invalid command. fd_out");
						exit(1);
					}
					if(dup2(fd_in,STDIN_FILENO)<0)
					{
						perror("Invalid command. dup2");
						exit(1);
					}
					close(fd_in);

				}

			if(j!=num_of_commands) //Except the Last command output has to be redirected to pipe
			{
				if(dup2(pipefileds[2*(j-1)+1],STDOUT_FILENO)<0)
				{

					std::cerr<<"When command is not Last"<<j<<"child_id_array[j-1"<<2*(j-1)+1<<std::endl;
					perror("Dup2 for pipe failed when command is not the last command");
					exit(1);
				}

			}

			if(j!=1)//Except for the first command input would be taken from the pipe just besides it
			{
				if(dup2(pipefileds[2*(j-2)],STDIN_FILENO)<0)
				{
					perror("Dup2 for pipe failed when command is not the first command");
					exit(1);
				}

			}

			for(int k=0;k<2*count_pipes;k++)
			{
				   close(pipefileds[k]);
			}

			if(execvp(list[0],&list[0])<0)
			{
				perror("Execvp failed");
				exit(1);
			}
		}
	 }

		for(int h=0;h<2*count_pipes;h++) //Parent closing all the file descriptors
		{
			close(pipefileds[h]);
		}

		for(int y=0;y<num_of_commands;y++)
		{
			if((waitpid(child_id_array[y],&status[y],0)) < 0)
			{
			   perror("wait");
			   return;
			}
			std::cout<<"exit status: "<<WEXITSTATUS(status[y])<<std::endl;

		}
		return;

}



void parse_and_run_command(std::vector<std::string> commandvector) {

	int outputrediflag=0,inputrediflag=0;;
	int outputredi=-1,inputredi=-1;
	int count_outputredi=0,count_inputredi=0;
	int status;
	char*  list[commandvector.size()+1];
	int index_oflist=0;

	for(int i=0;i<commandvector.size();i++)
	{
		if(commandvector[i].compare(">")==0)
		{
			//i!=commandvector.size()-1 is for the fact that commandvector[i+1] does not go out of bounds.
			if(i!=commandvector.size()-1 && (commandvector[i+1].compare(">")==0 || commandvector[i+1].compare("|")==0))
			{
				std::cerr<<"Invalid command."<<std::endl;
				return;
			}
			outputredi=i;
			outputrediflag=1;
			count_outputredi++;
			i++;
			continue;
		}
		
		if(commandvector[i].compare("<")==0)
		{
			if(i!=commandvector.size()-1 && (commandvector[i+1].compare(">")==0 || commandvector[i+1].compare("|")==0))
			{
				std::cerr<<"Invalid command."<<std::endl;
				return;
			}
			inputredi=i;
			inputrediflag=1;
			count_inputredi++;
			i++;
			continue;

		}

		list[index_oflist]=(&commandvector[i][0]);
		index_oflist++;

	}
	
	if(index_oflist ==0)//To check if command is null.
	{
		std::cerr<<"Invalid command."<<std::endl;
		return;
	}

	list[index_oflist]=(char *) 0;

	if(count_outputredi>1 || count_inputredi>1 || commandvector[commandvector.size()-1].compare("<")==0 ||
	   commandvector[commandvector.size()-1].compare(">")==0 || commandvector[commandvector.size()-1].compare("|")==0)
	{
		std::cerr<<"Invalid command."<<std::endl;
		return;
	}
	

	std::string tocheckexit=commandvector[0];
	if (tocheckexit.compare("exit")==0) 
	{
		exit(0);
	}
	
	int childid;
	childid=fork();
	if(childid < 0)
	{
		perror("Child could not be created, Please try later !");
		return;
	}
	else if(childid==0)
	{
		if(outputrediflag==1)
		{
			int fd_out=open(&commandvector[outputredi+1][0],O_CREAT | O_WRONLY | O_TRUNC,0644);
			if(fd_out<0)
			{
				perror("Invalid command. fd_out");
				exit(1);
			}
			
			if(dup2(fd_out,STDOUT_FILENO)<0)
			{
				perror("Invalid command. dup2");
				exit(1);
			}
			close(fd_out);
		}
		
		if(inputrediflag==1)
		{
			int fd_in=open(&commandvector[inputredi+1][0],O_RDONLY,0644);
			if(fd_in<0)
			{
				perror("Invalid command. fd_out");
				exit(1);
			}
			if(dup2(fd_in,STDIN_FILENO)<0)
			{
				perror("Invalid command. dup2");
				exit(1);
			}
			close(fd_in);
			
		}
		if(execv(list[0],&list[0])<0)
		{
			perror("Invalid command. Execv");
			exit(1);

		}
		exit(0);
	}
	else {
		if((wait(&status)) < 0){
			perror("wait");
			return;
		}
		std::cout<<"exit status: "<<WEXITSTATUS(status)<<std::endl;
		return;
	}
}

int main(void) {

    while (true)
    {
    	std::string command;
        std::cout << "> ";
        std::getline(std::cin, command);
        std::string token;
        std::vector <std::string> commandvector;
        int flagforpipe=0;
        
        if(command.empty())
		{

			std::cerr<<"Invalid command. 1"<<std::endl;
			continue;

		}

        std::istringstream s(command);
        while(s >> token)
        {
        	commandvector.push_back(token);
        	if(token.compare("|")==0)
        	{

        		flagforpipe=1;
        	}
        }
        if(commandvector.empty())
        {
        	std::cerr<<"Invalid command. 2"<<std::endl;
        	continue;
        }
        	if(flagforpipe != 1)
        		parse_and_run_command(commandvector);
        	else
        		parse_and_run_command_pipe(commandvector);
    }
    return 0;
}
