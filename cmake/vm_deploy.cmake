set( VM_VMX "$ENV{USERPROFILE}/Documents/Virtual Machines/Windows 11 x64/Windows 11 x64.vmx" CACHE FILEPATH "VMware VM" )
set( VM_CREDENTIAL "$ENV{USERPROFILE}/.codex/vmware/win11-vm.xml" CACHE FILEPATH "Encrypted VM credential" )
set( VM_GUEST_CREDENTIAL "$ENV{USERPROFILE}/.codex/vmware/win11-guest.xml" CACHE FILEPATH "Guest PSCredential" )
set( VM_DESTINATION "C:/Users/Public/dwm_hook-{hash}.dll" CACHE STRING "Guest DLL path" )
option( VM_AUTO_DEPLOY "Upload DLL after building" ON )
find_program( VMRUN vmrun PATHS "$ENV{ProgramFiles}/VMware/VMware Workstation" )
find_program( POWERSHELL NAMES pwsh powershell )
set( DEPLOY_COMMAND ${POWERSHELL} -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "${PROJECT_SOURCE_DIR}/vm_deploy.ps1"
    -File "$<TARGET_FILE:dwm_hook>" -Vmrun "${VMRUN}" -Vmx "${VM_VMX}"
    -VmCredential "${VM_CREDENTIAL}" -GuestCredential "${VM_GUEST_CREDENTIAL}" -Destination "${VM_DESTINATION}"
)
add_custom_target( vm_deploy COMMAND ${DEPLOY_COMMAND} DEPENDS dwm_hook USES_TERMINAL VERBATIM )
if( VM_AUTO_DEPLOY AND VMRUN AND POWERSHELL AND EXISTS "${VM_VMX}"
    AND EXISTS "${VM_CREDENTIAL}" AND EXISTS "${VM_GUEST_CREDENTIAL}" )
    add_custom_command( TARGET dwm_hook POST_BUILD COMMAND ${DEPLOY_COMMAND} USES_TERMINAL VERBATIM )
else( )
    message( STATUS "Automatic VM upload disabled; configure VM_* and build vm_deploy to upload manually" )
endif( )
