# BOOMERANG: Exploiting the Semantic Gap in Trusted Execution Environments
## Contents
1. [Introduction](#1-introduction)
2. [Folder Structure](#2-folder-structure)
    2. [Static Analysis Tool](#21-static-analysis-tool)     
    2. [Proof of Concept Exploits](#22-proof-of-concept-exploits)    
    2. [Cooperative Semantic Reconstruction](#23-cooperative-semantic-reconstruction)    
    2. [Papers](#24-papers)    
    2. [Presentation](#25-presentations)
3. [Contact](#3-contact)
4. [Coming Soon](#4-coming-soon)

## 1. Introduction
BOOMERANG , a class of vulnerabilities that stem from the semantic gap 
between the non-secure and secure worlds. BOOMERANG is a type of confused 
deputy attack, wherein a user-level application in the non-secure world can 
leverage a TA to read from or write to non-secure world memory that it does 
not own, including the untrusted OS’s. 

More specifically, the malicious user-level application can send inputs 
to the TA, which are not properly checked, that will trick the TA in to 
manipulating memory locations that should otherwise be inaccessible 
to the malicious application. BOOMERANG vulnerabilities can be used 
to steal or corrupt data in other user-level applications, or, in the worst 
case, completely compromise the untrusted OS. 

We found exploitable BOOMERANG vulnerabilities in four TEE implementations. 
These vulnerabilities were detected using a combination of manual analysis 
and an automated [static analysis tool](#21-static-analysis-tool), which is capable of locating potential 
vectors for exploiting BOOMERANG in a given TA. We were able to leverage 
vulnerabilities in two commercial TEE implementations to create [proof-of-concept exploits](#22-proof-of-concept-exploits).


---
## 2. Folder Structure
We now describe the different folders in this repo, along with their purpose.

### 2.1 Static Analysis Tool
### 2.2 Proof of Concept Exploits
### 2.3 Our defense: Cooperative Semantic Reconstruction (CSR)
### 2.4 Papers
### 2.5 Presentations

---
## 3. Contact
*Issues and Support:* boomerang-dev@googlegroups.com

*Non-support related:* machiry@cs.ucsb.edu

---
## 4. Coming Soon
