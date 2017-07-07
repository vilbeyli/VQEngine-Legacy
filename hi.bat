@echo off
echo Hello World from %AGENT_NAME%
echo ID: %AGENT_ID%
echo AGENT_WORKFOLDER contents:
@dir %AGENT_WORKFOLDER%
echo AGENT_BUILDDIRECTORY contents:
echo %BUILD_SOURCEDIRECTORY%
echo Over and out.
