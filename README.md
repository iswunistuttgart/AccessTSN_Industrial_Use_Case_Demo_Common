# AccessTSN Industrial Use Case Demo - Common
This is a demonstration to give an example on how to use Linux with the current support for Time Sensitive Networking (TSN) as an industrial endpoint. For this purpose the components of this demonstration form a converged network use case in an industrial setting. 

The Use Case Demonstration is aimed at the AccessTSN Endpoint Image and tested with it but should also work independently from the image.

This work is a result of the AccessTSN-Project. More information on the project as well as contact information are on the project's webpage: https://accesstsn.com

## The Industrial Use Case Demo - Common Git-project

The AccessTSN Industrial Use Case Demo consists of multiple Git-projects, each holding the code to one component of the demonstration. This is the *common* project. It includes the description ot the shared memory interface between the multiple components on the control system. it also includes two simple applications for reading and writing random values from/to the shared memory. Theses can be used during development and testing of the components of the AccessTSN Industrial Use Case Demo. In the future this project might also include common functions which are used in multiple projects of the AccessTSN Industrial Use Case Demo.

## Documentation

### Shared Memory Interface
In the file _mk_shminterface.h_ is the shared memory interface of the AccessTSN Industrial Use Case Demo defined. The available variables are clustered into 3 separate shared memories: Mainoutput and Maininput as well as Additionaloutput. Mainoutput contains the variables which are sent from the CNC to the axes and Maininput contains the variables sent to the CNC from the axes. Additionaloutput consists of variables outputted by the CNC which can be used in the HMI of for diagnostics and statistic purposed. In the Demo the Additionaloutput variables are not used int the RT-Control loop. 

For a list of the variables, their types and units please see the header file directly. 

### Demoreader / Demowriter ###
Demoreader and Demowriter are two simple applications which offer access to the AccessTSn Shared Memory Interfaces for development and testing purposes. Both open the specified shared memories or create them, if necessary. The also create a semaphore to be able to block the shared memories during writes. Demoreader output the formatted content of the shared memory periodically to the standard output. Demowriter periodically generated random values and writes them to the opened shared memories. The values are also formatted and outputted to the standard output. The generated random values **are not** valid CNC values, they might be out of range or contradict each other. In both application the length ot the period can be chosen though the CLI.

The CLI used following switches:
- -o : Choses the main output variables for read/write.
- -i : Choses the main input variables for read/write.
- -a : Choses the additional output variables for read/write.
- -t [value] : Specifies update-period in milliseconds. Default 10 seconds.
- -h : Prints the help message and exits.

The application can be build using the included Makefile. The files for the application can be found in the _demo_ subdirectory. 
