Frequently Asked Questions
==========================

estos Enhanced SNACC

## Contents

- [General](#general)
- [Licensing](#licensing)


## General

### Q: What is estos eSNACC?

A: estos eSNACC is the estos enhanced Sample Neufeld ASN.1 C Compiler.  It is a libre
   project with the goal of producing an ASN.1 to C/C++ (and more) code
   compiler.  ASN.1 is a standardized language maintained by the International
   Telecommunications Union.  The goal of ASN.1 is to produce a language which
   describes various types of data, and defines the framework by which that
   data is serialized and deserialized.
   The estos esnacc is a fork from https://github.com/esnacc/esnacc-ng and enhances
   the original project with:
  * Typescript (main maintained language)
	* Structure definitions
	* JSON and BER encoder/decoders
	* ROSE client implementation (complete stubs) for node and the   browser side
	* ROSE server implementation (complete stubs)
  * C++ (main maintained language)
	* Structure definitions
	* JSON and BER encoder/decoders
	* ROSE client/server implementation (complete stubs)
  * Delphi
	* Structure definitions
  * JAVA
	* Structure definitions
  * JavaScript JSON
	* Structure definitions
  * SWIFT
	* Structure definitions
  * JSDOC
	* JSON formatted Documentation generated based on the documentation inside the asn1 files

### Q: What versions of ASN.1 does the estos eSNACC interoperate with?

A: The estos eSNACC can support most of X.608/X.609 for Basic Encoding Rules.  Some
   notable exceptions exist (there are a few compliance issues with string
   formats and internationalization).
   
   
### Q: Which programming languages does the estos eSNACC target?

A: Currently, the estos eSNACC mainly targets C++ and typescript but other languages are also supported.
   Check the compiler back-ends directory for the currently adresses languages.
   However the different languages may not support the full featureset like the c++ and typescript implementation.


### Q: Which encoding rules are support?

A: This primarily depends on the programming language you are interested in.
   C++ and typescript both support BER (basic encoding rules) and JSON encoded structures.
   The other languages mainly only support JSON as they are lacking in open source BER encoders/decoders.

## Licensing

### Q: Why do you have such a strange set of licenses?

A: A bit of history.  The SNACC project began back when free licenses were not
   fully vetted.  The original code was licensed as both LGPL and GPL, but this
   was not workable (it would create a piece of code that was always GPL'd).

   It was relicensed for 1.1 as GPLv2 for the compiler, and "Free Software" for
   the library.  Although it is a bit fuzzy, the understanding is that this
   means you are free to run this work, modify this work, redistribute this
   work (including your modifications), and study the code this work (where
   this work means the parts of code which make up 'the library').

   In 2002/2003 timeframe, DigitalNet (a now defunct branch of Getronics) took
   the SNACC code and enhanced it to include better support for C++, and newer
   ASN.1 syntax.  These enhancements were added under the eSNACC Public License
   under contract to the US government.  They codify the 'Free Software' 
   license referenced above (although, obviously do not re-license the 
   software).
   
   The estos eSNACC continutes to use these licenses.


### Q: So am I free to use estos eSNACC in my application?

A: Yes.  If you incorporate the estos eSNACC compiler, you will be subject to the
   terms of the GNU GPL v2.  If you only use the compiler to generate code and
   then use the resulting code and support libraries, your new work can be 
   licensed however you like.