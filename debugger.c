#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "instruction.h"
#include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

#define MAX_LINE 256

// structure for linked list node
struct breakpoint
{
  uint64_t value;
  struct breakpoint *next;
};

struct breakpoint *head = NULL;
struct breakpoint *end = NULL;

static void addBreakpoint(uint64_t address);
static void deleteBreakpoint(uint64_t address);
static void deleteAllBreakpoints(void);
static int hasBreakpoint(uint64_t address);

int main(int argc, char **argv)
{

  int fd;
  struct stat st;

  machine_state_t state;
  y86_instruction_t nextInstruction;
  memset(&state, 0, sizeof(state));

  char line[MAX_LINE + 1], previousLine[MAX_LINE + 1] = "";
  char *command, *parameters;
  int c;

  // Verify that the command line has an appropriate number of
  // arguments
  if (argc < 2 || argc > 3)
  {
    fprintf(stderr, "Usage: %s InputFilename [startingPC]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to read, attempt to open it for
  // reading and verify that the open did occur.
  fd = open(argv[1], O_RDONLY);

  if (fd < 0)
  {
    fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
    return ERROR_RETURN;
  }

  if (fstat(fd, &st) < 0)
  {
    fprintf(stderr, "Failed to stat %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  state.programSize = st.st_size;

  // If there is a 2nd argument present it is an offset so convert it
  // to a numeric value.
  if (3 <= argc)
  {
    errno = 0;
    state.programCounter = strtoul(argv[2], NULL, 0);
    if (errno != 0)
    {
      perror("Invalid program counter on command line");
      close(fd);
      return ERROR_RETURN;
    }
    if (state.programCounter > state.programSize)
    {
      fprintf(stderr, "Program counter on command line (%lu) "
                      "larger than file size (%lu).\n",
              state.programCounter, state.programSize);
      close(fd);
      return ERROR_RETURN;
    }
  }

  // Maps the entire file to memory. This is equivalent to reading the
  // entire file using functions like fread, but the data is only
  // retrieved on demand, i.e., when the specific region of the file
  // is needed.
  state.programMap = mmap(NULL, state.programSize, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE, fd, 0);
  if (state.programMap == MAP_FAILED)
  {
    fprintf(stderr, "Failed to map %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  // Move to first non-zero byte
  while (!state.programMap[state.programCounter])
    state.programCounter++;

  printf("# Opened %s, starting PC 0x%lX\n", argv[1], state.programCounter);

  fetchInstruction(&state, &nextInstruction);
  printInstruction(stdout, &nextInstruction);

  while (1)
  {

    // Show prompt, but only if input comes from a terminal
    if (isatty(STDIN_FILENO))
      printf("> ");

    // Read one line, if EOF break loop
    if (!fgets(line, sizeof(line), stdin))
      break;

    // If line could not be read entirely
    if (!strchr(line, '\n'))
    {
      // Read to the end of the line
      while ((c = fgetc(stdin)) != EOF && c != '\n')
        ;
      if (c == '\n')
      {
        printErrorCommandTooLong(stdout);
        continue;
      }
      else
      {
        // In this case there is an EOF at the end of a line.
        // Process line as usual.
      }
    }

    // Obtain the command name, separate it from the arguments.
    command = strtok(line, " \t\n\f\r\v");
    // If line is blank, repeat previous command.
    if (!command)
    {
      strcpy(line, previousLine);
      command = strtok(line, " \t\n\f\r\v");
      // If there's no previous line, do nothing.
      if (!command)
        continue;
    }

    // Get the arguments to the command, if provided.
    parameters = strtok(NULL, "\n\r");

    sprintf(previousLine, "%s %s\n", command, parameters ? parameters : "");

    /* Quit or Exit */
    if (strcasecmp(command, "quit") == 0 || strcasecmp(command, "exit") == 0)
    {
      break;
    }

    /* Step */
    else if (strcasecmp(command, "step") == 0)
    {

      // Execute instruction at current program counter. Print instruction if
      // instruction is invalid.
      if (executeInstruction(&state, &nextInstruction) == 0)
      {
        printInstruction(stdout, &nextInstruction);
        continue;
      }
      // Fetch the next instruction and print it when current instruction is
      // executed.
      else
      {
        fetchInstruction(&state, &nextInstruction);
        printInstruction(stdout, &nextInstruction);
      }
    }

    /* Run */
    else if (strcasecmp(command, "run") == 0)
    {

      if (executeInstruction(&state, &nextInstruction) == 0)
      {
        printInstruction(stdout, &nextInstruction);
        continue;
      }
      else
      {
        fetchInstruction(&state, &nextInstruction);
        printInstruction(stdout, &nextInstruction);
      }

      // Repeated Execution
      while (!hasBreakpoint(state.programCounter) &&
             nextInstruction.icode != I_HALT &&
             nextInstruction.icode != I_INVALID)
      {

        // Execute current instruction. Print if invalid.
        // Fetch next instruction otherwise.
        if (executeInstruction(&state, &nextInstruction) == 0)
        {
          printInstruction(stdout, &nextInstruction);
          continue;
        }
        else
        {
          fetchInstruction(&state, &nextInstruction);
          printInstruction(stdout, &nextInstruction);
        }
      }
    }

    /* Next */
    else if (strcasecmp(command, "next") == 0)
    {

      // Instruction is not function call
      if (nextInstruction.icode != I_CALL)
      {

        // Same as Step command.
        if (executeInstruction(&state, &nextInstruction) == 0)
        {
          printf("In execute failure case.");
          printInstruction(stdout, &nextInstruction);
          continue;
        }
        else
        {
          printf("execute worked, calling fetch.");
          fetchInstruction(&state, &nextInstruction);
          printf("execute worked, fetch worked.");
          printInstruction(stdout, &nextInstruction);
        }
      }
      else
      {

        // Save stack pointer
        uint64_t stackPointer = state.registerFile[R_RSP];

        // Repeated execution
        while (!hasBreakpoint(state.programCounter) &&
               nextInstruction.icode != I_HALT &&
               nextInstruction.icode != I_INVALID)
        {

          if (executeInstruction(&state, &nextInstruction) == 0)
          {
            printInstruction(stdout, &nextInstruction);
            break;
          }
          else
          {
            fetchInstruction(&state, &nextInstruction);

            // Break if the function has returned
            if (stackPointer == state.registerFile[R_RSP])
            {
              printInstruction(stdout, &nextInstruction); // doubtful
              break;
            }
          }
        }
      }
    }

    /* Jump */
    else if (strcasecmp(command, "jump") == 0)
    {
      // prints invalid command if address parameter provided
      if (!parameters)
      {
        printErrorInvalidCommand(stdout, command, parameters);
        continue;
      }
      else
      {
        uint64_t address = strtoul(parameters, NULL, 16);
        state.programCounter = address;
        fetchInstruction(&state, &nextInstruction);
        printInstruction(stdout, &nextInstruction);
      }
    }

    /* Break */
    else if (strcasecmp(command, "break") == 0)
    {
      if (parameters)
      {
        uint64_t address = strtoul(parameters, NULL, 16);
        addBreakpoint(address);
      }
    }

    /* Delete */
    else if (strcasecmp(command, "delete") == 0)
    {
      if (parameters)
      {
        uint64_t address = strtoul(parameters, NULL, 16);
        deleteBreakpoint(address);
      }
    }

    /* Registers */
    else if (strcasecmp(command, "registers") == 0)
    {
      for (int i = R_RAX; i < R_NONE; ++i)
      {
        printRegisterValue(stdout, &state, i);
      }
    }

    /* Examine */
    else if (strcasecmp(command, "examine") == 0)
    {
      // print invalid command if address parameter not provided
      if (!parameters)
      {
        printErrorInvalidCommand(stdout, command, parameters);
      }
      else
      {
        uint64_t address = strtoul(parameters, NULL, 16);
        printMemoryValueQuad(stdout, &state, address);
      }
    }

    /* Command not listed above */
    else
    {
      printErrorInvalidCommand(stdout, command, parameters);
    }
  }

  /* Close all resources, delete breakpoints and terminate debugger */
  deleteAllBreakpoints();
  munmap(state.programMap, state.programSize);
  close(fd);
  return SUCCESS;
}

/* Adds an address to the list of breakpoints. If the address is
 * already in the list, it is not added again. */
static void addBreakpoint(uint64_t address)
{

  if (hasBreakpoint(address) == 0)
  {
    struct breakpoint *newBreakpoint = (struct breakpoint *)malloc(sizeof(struct breakpoint));
    newBreakpoint->value = address;
    newBreakpoint->next = NULL;

    if (head == NULL && end == NULL)
    {
      head = newBreakpoint;
      end = newBreakpoint;
    }
    else
    {
      end->next = newBreakpoint;
      end = newBreakpoint;
    }
  }
}

/* Deletes an address from the list of breakpoints. If the address is
 * not in the list, nothing happens. */
static void deleteBreakpoint(uint64_t address)
{
  struct breakpoint *pointer = head;
  struct breakpoint *previous = NULL;

  while (pointer != NULL)
  {
    if (pointer->value == address)
    {
      if (previous == NULL && pointer == head)
      {
        previous = head;
        head = head->next;
      }
      else
      {
        previous->next = pointer->next;
      }
      free(pointer);
      pointer = NULL;
      break;
    }
    previous = pointer;
    pointer = pointer->next;
  }
}

/* Deletes and frees all breakpoints. */
static void deleteAllBreakpoints(void)
{

  struct breakpoint *pointer = head;
  struct breakpoint *temp = NULL;

  while (pointer != NULL)
  {
    temp = pointer->next;
    free(pointer);
    pointer = temp;
  }

  head = NULL;
  end = NULL;
}

/* Returns true (non-zero) if the address corresponds to a breakpoint
 * in the list of breakpoints, or false (zero) otherwise. */
static int hasBreakpoint(uint64_t address)
{

  struct breakpoint *pointer = head;
  int found = 0;

  while (pointer != NULL)
  {
    if (pointer->value == address)
    {
      found = 1;
      break;
    }
    else
    {
      pointer = pointer->next;
    }
  }

  return found;
}
