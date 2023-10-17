@echo off
IF NOT EXIST "../../../../../../../github/ASN1.ts" (
	echo checkouf asn1ts to X:\dev\github\ASN1.ts
	pause
	exit 1
)

cd ..
set local=%CD%
cd ../../../../../../github/ASN1.ts
echo Linking source repository %CD%
echo npm link
cmd /c npm link

cd %local%
echo Linking target repository %CD%
echo npm link @estos/asn1ts
cmd /c npm link @estos/asn1ts

pause
