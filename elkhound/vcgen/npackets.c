#define STATUS_SUCCESS 1
#define STATUS_UNSUCCESSFUL 0

struct Irp {
	int Status;
	int Information;
};

struct Requests {
	int Status;
	struct Irp *irp;
	struct Requests *Next;	
};

struct Device {
	struct Requests *WriteListHeadVa; 
	int writeListLock;
};

void Lock() {
    __AddEvent(1,"lock",1);
}
void Unlock() {
    __AddEvent(-1,"lock",1);
}

// stubs
void SmartDevFreeBlock(struct Requests *r) {
}

void IoCompleteRequest(struct Irp *irp, int status) {
}

struct Device devE;

int main () {
    int IO_NO_INCREMENT = 3;
    int nPacketsOld, nPackets;
    struct Requests r, *request=&r;
    struct Irp i, *irp = &i;
    struct Device d, *devExt = &d;

    do {
        Lock();

        nPacketsOld = nPackets; 

        request = (*devExt).WriteListHeadVa;
        if(request!=0 && (*request).Status!=0){
            (*devExt).WriteListHeadVa = (*request).Next;	

            Unlock();
            irp = (*request).irp;
            if((*request).Status >0) {
                (*irp).Status = STATUS_SUCCESS;
                (*irp).Information = (*request).Status;
            } else {
                (*irp).Status = STATUS_UNSUCCESSFUL;
                (*irp).Information = (*request).Status;
            }	
            SmartDevFreeBlock(request);
            IO_NO_INCREMENT = 3;
            IoCompleteRequest(irp, IO_NO_INCREMENT);

            nPackets = nPackets + 1;
        }
    } while (nPackets != nPacketsOld); 

    Unlock();
    return 0;
}
