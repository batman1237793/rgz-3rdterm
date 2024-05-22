#include <windows.h> 
#include <stdio.h> 
#include <cstdlib>
#include <aclapi.h>

int AddLogMessage(const char* message, int errorCode = 0);

int WorkWithClient();

void WaitClient();

int ServiceStart();

void ServiceStop();

int StartEvents();
#pragma once
