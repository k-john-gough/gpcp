REM This batch file for .NET Version 2.0 +
REM build the .NET system CP symbol files
..\..\..\bin\PeToCps mscorlib.dll
..\..\..\bin\PeToCps System.dll
..\..\..\bin\PeToCps System.Drawing.dll
..\..\..\bin\PeToCps System.Security.dll
..\..\..\bin\PeToCps /big System.Windows.Forms.dll
..\..\..\bin\PeToCps System.XML.dll
..\..\..\bin\PeToCps System.Data.dll
..\..\..\bin\PeToCps System.Configuration.dll
REM and then the corresponding HTML Browse files
..\..\..\bin\Browse /html /sort mscorlib_Microsoft_Win32.cps
..\..\..\bin\Browse /html /sort mscorlib_System.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Collections.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Configuration_Assemblies.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Diagnostics.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Diagnostics_SymbolStore.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Globalization.cps
..\..\..\bin\Browse /html /sort mscorlib_System_IO.cps
..\..\..\bin\Browse /html /sort mscorlib_System_IO_IsolatedStorage.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Reflection.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Reflection_Emit.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Resources.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_CompilerServices.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_InteropServices.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_InteropServices_Expando.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Activation.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Channels.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Contexts.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Lifetime.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Messaging.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Metadata.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Metadata_W3cXsd2001.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Proxies.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Remoting_Services.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Serialization.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Serialization_Formatters.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Runtime_Serialization_Formatters_Binary.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Security.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Security_Cryptography.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Security_Cryptography_X509Certificates.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Security_Permissions.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Security_Policy.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Security_Principal.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Text.cps
..\..\..\bin\Browse /html /sort mscorlib_System_Threading.cps
..\..\..\bin\Browse /html /sort System_.cps
..\..\..\bin\Browse /html /sort System_Drawing_.cps
..\..\..\bin\Browse /html /sort System_Drawing__Design.cps
..\..\..\bin\Browse /html /sort System_Drawing__Drawing2D.cps
..\..\..\bin\Browse /html /sort System_Drawing__Imaging.cps
..\..\..\bin\Browse /html /sort System_Drawing__Printing.cps
..\..\..\bin\Browse /html /sort System_Drawing__Text.cps
..\..\..\bin\Browse /html /sort System_Microsoft_CSharp.cps
..\..\..\bin\Browse /html /sort System_Microsoft_VisualBasic.cps
..\..\..\bin\Browse /html /sort System_Microsoft_Win32.cps
..\..\..\bin\Browse /html /sort System_Security__Cryptography_Xml.cps
..\..\..\bin\Browse /html /sort System_Windows_Forms_.cps
..\..\..\bin\Browse /html /sort System_Windows_Forms_System_Resources.cps
..\..\..\bin\Browse /html /sort System_Windows_Forms__ComponentModel_Com2Interop.cps
..\..\..\bin\Browse /html /sort System_Windows_Forms__Design.cps
..\..\..\bin\Browse /html /sort System_Windows_Forms__PropertyGridInternal.cps
..\..\..\bin\Browse /html /sort System__CodeDom.cps
..\..\..\bin\Browse /html /sort System__CodeDom_Compiler.cps
..\..\..\bin\Browse /html /sort System__Collections_Specialized.cps
..\..\..\bin\Browse /html /sort System__ComponentModel.cps
..\..\..\bin\Browse /html /sort System__ComponentModel_Design.cps
..\..\..\bin\Browse /html /sort System__ComponentModel_Design_Serialization.cps
..\..\..\bin\Browse /html /sort System__Configuration.cps
..\..\..\bin\Browse /html /sort System__Diagnostics.cps
..\..\..\bin\Browse /html /sort System__IO.cps
..\..\..\bin\Browse /html /sort System__Net.cps
..\..\..\bin\Browse /html /sort System__Net_Sockets.cps
..\..\..\bin\Browse /html /sort System__Security_Cryptography_X509Certificates.cps
..\..\..\bin\Browse /html /sort System__Security_Permissions.cps
..\..\..\bin\Browse /html /sort System__Text_RegularExpressions.cps
..\..\..\bin\Browse /html /sort System__Threading.cps
..\..\..\bin\Browse /html /sort System__Timers.cps
..\..\..\bin\Browse /html /sort System__Web.cps
..\..\..\bin\Browse /html /sort System_Xml_.cps
..\..\..\bin\Browse /html /sort System_Xml__Schema.cps
..\..\..\bin\Browse /html /sort System_Xml__XPath.cps
..\..\..\bin\Browse /html /sort System_Xml__Xsl.cps
..\..\..\bin\Browse /html /sort System_Xml__Serialization.cps
..\..\..\bin\Browse /html /sort System_Xml__Serialization_Advanced.cps
..\..\..\bin\Browse /html /sort System_Xml__Serialization_Configuration.cps
..\..\..\bin\Browse /html /sort System_Data_.cps
..\..\..\bin\Browse /html /sort System_Data__Common.cps
..\..\..\bin\Browse /html /sort System_Data__Odbc.cps
..\..\..\bin\Browse /html /sort System_Data__OleDb.cps
..\..\..\bin\Browse /html /sort System_Data__SqlClient.cps
..\..\..\bin\Browse /html /sort System_Data__SqlTypes.cps
..\..\..\bin\Browse /html /sort System_Data_System_Xml.cps
..\..\..\bin\Browse /html /sort System_Configuration_.cps

