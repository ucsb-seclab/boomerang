# BOOMERANG: Exploiting the Semantic Gap in Trusted Execution Environments
## Contents
1. [Introduction](#1-introduction)
2. [Folder Structure](#2-folder-structure)

    2.1 [Static Analysis Tool](#21-static-analysis-tool)     
    2.2 [Proof of Concept Exploits](#22-proof-of-concept-exploits)    
    2.3 [Cooperative Semantic Reconstruction](#23-cooperative-semantic-reconstruction)    
    2.4 [Papers](#24-papers)    
    2.5 [Presentation](#25-presentations)
3. [Contact](#3-contact)
4. [Impact](#4-impact)
5. [Coming Soon](#5-coming-soon)

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
We created a static analysis technique to locate exploitable BOOMERANG flaws using simulated execution, which we implemented using the angr static analysis and reverse-engineering framework.
This tool currently supports TAs of QSEE and Huawei..

More details can be found at [static_analysis_tool](https://github.com/ucsb-seclab/boomerang/tree/master/static_analysis_tool)
### 2.2 Proof of Concept Exploits
We developed a proof-of-concept arbitrary memory leak (by using physical address) on Qualcomm chipsets and privilege-escalation exploit on Huawei chipsets to verify the hypothesized severity of BOOMERANG.

More details can be found at [exploits](https://github.com/ucsb-seclab/boomerang/tree/master/exploits)
### 2.3 Cooperative Semantic Reconstruction
Due to the limitations of existing BOOMERANG defenses, we propose a novel defense Cooperative Semantic Reconstruction (CSR), which is capable of bridging the semantic gap between the two worlds with minimal modification and minimal overhead. In this defense, the trusted OS and the untrusted OS both cooperate to verify memory pointers that are passed into the secure world to ensure that the untrusted application indeed has permission to access the referenced memory region.

We used [OP-TEE](https://github.com/OP-TEE/optee_os) to implement and evaluate our defense. More details can be found at: [optee](https://github.com/ucsb-seclab/boomerang/tree/master/optee)
### 2.4 Papers
All papers and other academic publications based on this can be found under [papers](https://github.com/ucsb-seclab/boomerang/tree/master/papers).
### 2.5 Presentations
All the presentations at various venues can be found under [presentations](https://github.com/ucsb-seclab/boomerang/tree/master/presentations)

---
## 3. Contact
*Issues and Support:* boomerang-dev@googlegroups.com

*Non-support related:* machiry@cs.ucsb.edu

---
## 4. Impact
These vulnerabilities, and our corresponding exploits, affect hundreds of millions of devices that are currently in production today.
Our findings have been reported to and verified by the corresponding vendors, who are currently in the process of creating security patches.

CVEs Issued: **CVE-2016-5349, CVE-2016-8762, CVE-2016-8763,** and **CVE-2016-8764**

---
## 5. Coming Soon
A patch file with all the modifications we have done to OP-TEE, so that its easy to port our defense to different versions of OP-TEE.

