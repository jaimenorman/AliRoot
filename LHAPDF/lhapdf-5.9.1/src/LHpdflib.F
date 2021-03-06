! -*- F90 -*-


! Initialize a PDF set, determining the path to the PDF set directory automatically
! subroutine InitPDFsetByCodes(code1, code2, code3)
!   write(*,*) "Not implemented yet: this will move the 'glue' interface to ", &
!        "LHAPDF proper and use the InitPDFsetByName function to ", &
!        "get the path automatically."
!   return
! end subroutine InitPDFsetByCodes



! Initialize a PDF set, determining the path to the PDF set
! directory automatically
subroutine InitPDFsetByName(setname)
  implicit none
  character setname*(*)
  integer nset
  nset = 1
  call commoninit()
  call InitPDFsetByNameM(nset,setname)
  return
end subroutine InitPDFsetByName



! Initialize a PDF set, determining the path to the PDF set
! directory automatically
subroutine InitPDFsetByNameM(nset,setname)
  implicit none
  include 'parmsetup.inc'
  include 'commonlhapdfc.inc'
  include 'commonlhacontrol.inc'
  character setname*(*)
  integer nset
  character*512 dirpath, setpath

  ! Initialise common blocks
  call commoninit()

  ! Find the directory with the PDFsets
  call getdirpath(dirpath)

  ! Now build the path to the PDF set
  setpath = dirpath(:len_trim(dirpath)) // "/" // setname(:len_trim(setname))

  ! Initialize using the detected PDF set
  call InitPDFsetM(nset, setpath(:len_trim(setpath)))
  return
end subroutine InitPDFsetByNameM



subroutine InitPDFset(setpath)
  implicit none
  integer nset
  character setpath*(*)
  nset = 1
  call commoninit()
  call InitPDFsetM(nset,setpath)
  return
end subroutine InitPDFset


subroutine InitLHAPDF()
  call commoninit()
end subroutine InitLHAPDF


subroutine InitPDFsetM(nset,setpath)
  implicit none
  include 'parmsetup.inc'
  include 'commonlhacontrol.inc'
  character*512 filename
  common/lhafilename/filename
  character setpath*(*)
  character*512 inputfile(nmxset)
  character*64 string
  character*16 s1,s2
  character*10 lhaversion
  integer id,token,Ctoken
  integer lhaonce
  save lhaonce,inputfile
  data lhaonce/0/
  integer lhasilent
  common/lhasilent/lhasilent
  integer nset,nnset
  integer stat

  filename=setpath
#ifndef LOW_MEMORY
  ! check if this set is already initialized - will not work for lite
  if(inputfile(nset)(:len_trim(setpath)).eq.setpath) return
#endif

  ! Initialise common blocks
  call commoninit()
  call getlhapdfversion(lhaversion)

  inputfile(nset)=setpath
  lhasilent = 0
  if (lhaparm(19).eq.'SILENT') then
     lhasilent = 1
  elseif (lhaparm(19).eq.'LOWKEY') then
     if (lhaonce .eq. 0) then
        lhaonce = 1
     else
        lhasilent = 1
     endif
  endif

  call setnset(nset)
  open(unit=1, file=setpath, status='old', iostat=stat)
  if (stat .ne. 0) then
    write(*,*) 'File ', setpath(:len_trim(setpath)), ' cannot be opened !'
    write(*,*) 'If you have not already done so:'
    write(*,*) 'Use the bin/lhapdf-getdata script to download the file.'
    write(*,*) 'Set the environmental variable LHAPATH to specify the directory if not as default (above).'
    close(1)
    call exit(1)
  end if
  read(1,*) s1,s2
  if ((    index(s2,'1.0').ne.1) &
     .and.(index(s2,'1.1').ne.1) &
     .and.(index(s2,'2.0').ne.1) &
     .and.(index(s2,'2.1').ne.1) &
     .and.(index(s2,'3.0').ne.1) &
     .and.(index(s2,'3.1').ne.1) &
     .and.(index(s2,'4.0').ne.1) &
     .and.(index(s2,'5.0').ne.1) &
     .and.(index(s2,'5.3').ne.1) &
     .and.(index(s2,'5.4').ne.1) &
     .and.(index(s2,'5.5').ne.1) &
     .and.(index(s2,'5.6').ne.1) &
     .and.(index(s2,'5.7').ne.1) &
     .and.(index(s2,'5.8').ne.1)) then
     write(*,*) 'Version ',s2,' not supported by this version of LHAPDF'
     stop
  else
     if (lhasilent.eq.0) then
        write(*,*) '*************************************'
        write(*,*) '*       LHAPDF Version ',lhaversion,'   *'
        write(*,*) '*   Configured for the following:   *'
#ifdef ALL
        write(*,*) '*             All PDFs              *'
#endif
#ifndef ALL
#ifdef MRST
        write(*,*) '*             MRSTMAIN              *'
#endif
#ifdef MRST06
        write(*,*) '*              MRST06               *'
#endif
#ifdef MRST98
        write(*,*) '*              MRST98               *'
#endif
#ifdef MRSTQED
        write(*,*) '*              MRSTQED              *'
#endif
#ifdef CTEQ
        write(*,*) '*               CTEQ                *'
#endif
#ifdef MSTW
        write(*,*) '*               MSTW                *'
#endif
#ifdef ALEKHIN
        write(*,*) '*              ALEKHIN              *'
#endif
#ifdef NNPDF
        write(*,*) '*               NNPDF               *'
#endif
#ifdef BOTJE
        write(*,*) '*               BOTJE               *'
#endif
#ifdef FERMI
        write(*,*) '*               FERMI               *'
#endif
#ifdef ZEUS
        write(*,*) '*               ZEUS                *'
#endif
#ifdef H1
        write(*,*) '*                H1                 *'
#endif
#ifdef HERA
        write(*,*) '*               HERA                *'
#endif
#ifdef GRV
        write(*,*) '*                GRV                *'
#endif
#ifdef GJR
        write(*,*) '*               GJR/JR              *'
#endif
#ifdef HKN
        write(*,*) '*                HKN                *'
#endif
#ifdef PIONS
        write(*,*) '*               PIONS               *'
#endif
#ifdef PHOTONS
        write(*,*) '*              PHOTONS              *'
#endif
#ifdef USER
        write(*,*) '*               USER                *'
#endif
#endif
#ifdef LOW_MEMORY
        write(*,*) '*          LOW MEMORY option        *'
#endif
#ifdef FULL_MEMORY
        write(*,*) '*         FULL MEMORY option        *'
#endif
 1111 format(' *    Maximum ',i2,' concurrent set(s)   *')
        write(*,1111),nmxset
        write(*,*) '*************************************'
        write(*,*)
     endif
  endif
  id=Ctoken()
1 read(1,*) string
  id=token(string)
  ! print *,'id = ',id,string
  if (id.eq.0) then
     write(*,*) 'File description error:'
     write(*,*) 'Command not understood: ',string
     stop
  endif
  if (id.eq.1) call descriptionPDF(nset,id)
  ! print *,'1/2'
  if (id.eq.2) then 
     call initEvolve(nset)
  endif
  ! print *,'2/3'
  if (id.eq.3) call initAlphasPDF(nset)
  ! print *,'3/4'
  if (id.eq.4) call initInputPDF(nset)
  ! print *,'4/5'
  if (id.eq.5) call initListPDF(nset)
  ! print *,'5/6'
  if (id.eq.6) call initQCDparams(nset)
  ! print *,'6/7'
  if (id.eq.7) call initMinMax(nset)
  ! print *,'7/8'
  if (id.ne.8) goto 1
  close(1)
  ! print *,'calling InitEvolveCode',nset
  call InitEvolveCode(nset)

  ! Initialize the default member 0
  call InitPDFM(nset,0)

  return
  entry getsetpath(setpath)
    call getnset(nnset)
    setpath=inputfile(nnset)
  return

end subroutine InitPDFsetM



integer function token(s)
  implicit none
  character*16 s
  integer not,i,Ctoken
  parameter(not=8)
  character*16 t(not)
  data t/'Description:','Evolution:','Alphas:', 'Parametrization:', &
       'Parameterlist:','QCDparams:','MinMax:','End:'/
  integer count(not)
  save count

  token=0
  do i=1,not
     if (s.eq.t(i)) token=i
  enddo
  if (token.ne.0) then
     count(token)=count(token)+1
     if (count(token).eq.2) then
        write(*,*) 'File description error:'
        write(*,*) 'Second definition of entry: ',s
        stop
     endif
  endif
  return

  entry Ctoken()
  do i=1,not
     count(i)=0
  enddo
  Ctoken=0
  return
end function token



subroutine LHAprint(iprint)
  implicit none
  include 'commonlhacontrol.inc'
  integer lhasilent,iprint
  common/lhasilent/lhasilent
  call commoninit()
  lhasilent = iprint
  ! If using stream #6, don't silence!
  if(iprint.ne.6) lhaparm(19)='SILENT'
  return
end subroutine LHAprint



subroutine setPDFpath(pathname)
  implicit none
  include 'commonlhapdfc.inc'
  include 'commonlhacontrol.inc'
  include 'parmsetup.inc'
  character*(*) pathname
  integer j

  call commoninit()
  lhaparm(20) = 'LHAPATH'
  do j=1,len_trim(lhapath)
     lhapath(j:j)=''
  enddo
  lhapath = pathname
  return
end subroutine setPDFpath



subroutine lhaset(lhaparm2,lhavalue2)
  implicit none
  include 'commonlhacontrol.inc'
  character*20 lhaparm2(20)
  double precision lhavalue2(20)
  integer j

  call commoninit()
  do j=1,20
     lhaparm(j)=lhaparm2(j)
     lhavalue(j)=lhavalue2(j)
  enddo
  return
end subroutine lhaset



subroutine setlhaparm(lparm)
  implicit none
  include 'commonlhacontrol.inc'
  character*(*) lparm
  integer nparm

  call commoninit()

  if(lparm.eq.'EKS98') then
     lhaparm(15)='EKS98'
  else if(lparm.eq.'EPS08') then
     lhaparm(15)='EPS08'
  else if(lparm.eq.'EPS09') then
     lhaparm(15)='EPS09'
  else if(lparm(1:5).eq.'EPS09') then
     lhaparm(15)=lparm(1:LEN_TRIM(lparm))
  else if(lparm.eq.'15') then
     lhaparm(15)=''
  else if(lparm.eq.'NOSTAT') then
     lhaparm(16)='NOSTAT'
  else if (lparm.eq.'16') then
     lhaparm(16)=''
  else if (lparm.eq.'LHAPDF') then
     lhaparm(17)='LHAPDF'
  else if (lparm.eq.'17') then
     lhaparm(17)=''
  else if (lparm.eq.'EXTRAPOLATE') then
     lhaparm(18)='EXTRAPOLATE'
  else if (lparm.eq.'18') then
     lhaparm(18)=''
  else if (lparm.eq.'SILENT') then
     lhaparm(19)='SILENT'
  else if (lparm.eq.'LOWKEY') then
     lhaparm(19)='LOWKEY'
  else if (lparm.eq.'19') then
     lhaparm(19)=''
  else
     print *,'WARNING from SetLHAPARM - value',lparm,'not recognized!'
  endif
  return

  entry getlhaparm(nparm,lparm)
  lparm = lhaparm(nparm)
  return
end subroutine setlhaparm



subroutine getdirpath(dirpath)
  ! This routine is to determine the directory path for the PDFsets
  ! directory. It has a two-fold purpose:
  ! 1) to return the value as an argument in dirpath for the native
  !    LHAPDF use (ie via initPDFSetByName
  ! 2) to fill the value of lhapath in the LHAPDFC common for use in
  !    lhaglue.
  implicit none
  include 'commonlhapdfc.inc'
  include 'commonlhacontrol.inc'
  include 'parmsetup.inc'
  character*(*) dirpath

  ! First look in the LHAPDFC array (lhaparm(20), set by setPDFpath).
  ! Next, check environmental variable LHAPATH.
  ! Finally, use binreloc via getdatapath(...).
  ! Will use default path if this all fails.
  if (lhaparm(20) /= 'LHAPATH') then
     call getenv('LHAPATH', lhapath)
     !call get_environment_variable('LHAPATH',lhapath)
     if (lhapath.eq.'') then
        call getdatapath(dirpath)
        lhapath = dirpath
     endif
  endif
  dirpath = lhapath
  return
end subroutine getdirpath


!-- Get the maximum number of concurrent PDF sets.
subroutine GetMaxNumSets(MaxNumSets)
  implicit none
  include 'parmsetup.inc'
  integer MaxNumSets
  MaxNumSets = nmxset
end subroutine GetMaxNumSets


logical function has_photon()
  implicit none
  include 'parmsetup.inc'
  integer nset
  character*16 name(nmxset)
  integer nmem(nmxset),ndef(nmxset),mem
  common/NAME/name,nmem,ndef,mem
!
  call getnset(nset)
  has_photon = .FALSE.

  if(name(nset).eq.'MRST4qed') has_photon = .TRUE.
  if(name(nset).eq.'NNPDF20intqed') has_photon = .TRUE.
!
  return
!
end function has_photon
