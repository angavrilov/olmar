// npktsmall.c
// very simplified version of npackets.c
// NUMERRORS 1

// model lock state as a single variable
int lockState;

void Lock()
  thmprv_pre lockState == 0;
  thmprv_post lockState == 1;;
//{
//  lockState = 1;
//}

void Unlock()
  thmprv_pre lockState == 1;
  thmprv_post lockState == 0;;
//{
//  lockState = 0;
//}

// nondeterministic choice
int choice()
  thmprv_pre
    int pre_lockState = lockState; true;
  thmprv_post
    lockState == pre_lockState;
  ;

int main ()
  thmprv_pre lockState == 0;
  thmprv_post lockState == 0;
{
    int nPacketsOld, nPackets;

    do {
        thmprv_invariant lockState == 0;

        Lock();

        nPacketsOld = nPackets;

        if (choice()) {
            Unlock();

            nPackets = nPackets + 1;
            nPackets = nPacketsOld;      // ERROR(1)
        }
    } while (nPackets != nPacketsOld);

    Unlock();
    return 0;
}
