#define _NTDEF_ // Required to include both Ntsecapi and Winternl
#include <Winternl.h>
#include <codecvt>
#include <crypt.hpp>
#include <iomanip>
#include <iostream>
#include <lsa.hpp>
#include <msv1_0.hpp>
#include <netlogon.hpp>
#include <string>
#include <vector>

#define STATUS_SUCCESS 0

namespace {
    // The Rtl* functions were dynamically resolved to save time during development
    PSID MakeDomainRelativeSid(PSID DomainId, ULONG RelativeId) {
        PSID result{ nullptr };
        auto library{ LoadLibraryW(L"ntdll.dll") };
        if (library) {
            using PRtlCopySid = NTSTATUS (*)(ULONG DestinationSidLength, PSID DestinationSid, PSID SourceSid);
            auto RtlCopySid{ reinterpret_cast<PRtlCopySid>(GetProcAddress(library, "RtlCopySid")) };
            using PRtlLengthRequiredSid = ULONG (*)(ULONG SubAuthorityCount);
            auto RtlLengthRequiredSid{ reinterpret_cast<PRtlLengthRequiredSid>(GetProcAddress(library, "RtlLengthRequiredSid")) };
            using PRtlSubAuthorityCountSid = PUCHAR (*)(PSID pSid);
            auto RtlSubAuthorityCountSid{ reinterpret_cast<PRtlSubAuthorityCountSid>(GetProcAddress(library, "RtlSubAuthorityCountSid")) };
            using PRtlSubAuthoritySid = LPDWORD (*)(PSID pSid, DWORD nSubAuthority);
            auto RtlSubAuthoritySid{ reinterpret_cast<PRtlSubAuthoritySid>(GetProcAddress(library, "RtlSubAuthoritySid")) };
            if (RtlCopySid && RtlLengthRequiredSid && RtlSubAuthorityCountSid && RtlSubAuthoritySid) {
                auto subAuthorityCount{ *(RtlSubAuthorityCountSid(DomainId)) }; // Should not fail
                auto length{ RtlLengthRequiredSid(subAuthorityCount + 1) }; // Should not fail
                auto sid{ reinterpret_cast<PSID>(std::malloc(length)) }; // Assume this succeeds for brevity
                if (SUCCEEDED(RtlCopySid(length, sid, DomainId))) {
                    (*(RtlSubAuthorityCountSid(sid)))++;
                    *RtlSubAuthoritySid(sid, subAuthorityCount) = RelativeId;
                    result = sid;
                }
            }
            FreeLibrary(library);
        }
        return result;
    }

    constexpr size_t RoundUp(size_t count, size_t powerOfTwo) {
        return (count + powerOfTwo - 1) & (~powerOfTwo - 1);
    }
}

namespace Msv1_0 {
    Proxy::Proxy(const std::shared_ptr<Lsa>& lsa)
        : lsa(lsa) {
    }

    bool Proxy::CacheLogon(void* logonInfo, void* validationInfo, const std::vector<byte>& supplementalCacheData, ULONG flags) const {
        CACHE_LOGON_REQUEST request;
        request.LogonInformation = logonInfo;
        request.ValidationInformation = validationInfo;
        request.SupplementalCacheData = const_cast<byte*>(supplementalCacheData.data());
        request.SupplementalCacheDataLength = supplementalCacheData.size();
        void* response;
        return CallPackage(request, &response);
    }

    bool Proxy::CacheLookupEx(const std::wstring username, const std::wstring domain, CacheLookupCredType type, const std::string credential) const {
        CACHE_LOOKUP_EX_REQUEST request;
        UnicodeString userName{ username };
        request.UserName = userName;
        UnicodeString domainName{ domain };
        request.DomainName = domainName;
        request.CredentialType = type;
        request.CredentialInfoLength = credential.length();
        //&request.CredentialSubmitBuffer = credential.data();
        CACHE_LOOKUP_EX_RESPONSE* response;
        //auto result{ CallPackage(request, credential.length() - 1, &response) };
        if (1) {
            //response.
        }
        return 1;
    }

    bool Proxy::ChangeCachedPassword(const std::wstring& domainName, const std::wstring& accountName, const std::wstring& oldPassword, const std::wstring& newPassword, bool impersonating) const {
        CHANGE_CACHED_PASSWORD_REQUEST request;
        UnicodeString domainNameGuard{ domainName };
        request.DomainName = domainNameGuard;
        UnicodeString accountNameGuard{ accountName };
        request.AccountName = accountNameGuard;
        UnicodeString oldPasswordGuard{ oldPassword };
        request.OldPassword = oldPasswordGuard;
        UnicodeString newPasswordGuard{ newPassword };
        request.NewPassword = newPasswordGuard;
        request.Impersonating = impersonating;
        CHANGE_CACHED_PASSWORD_RESPONSE* response;
        auto result{ CallPackage(request, &response) };
        if (result) {
            lsa->out << "PasswordInfoValid    : " << response->PasswordInfoValid << std::endl;
            auto& DomainPasswordInfo{ response->DomainPasswordInfo };
            lsa->out << "MinPasswordLength    : " << DomainPasswordInfo.MinPasswordLength << std::endl;
            lsa->out << "PasswordHistoryLength: " << DomainPasswordInfo.PasswordHistoryLength << std::endl;
            lsa->out << "PasswordProperties   : " << DomainPasswordInfo.PasswordProperties << std::endl;
            lsa->out << "MaxPasswordAge       : " << DomainPasswordInfo.MaxPasswordAge.QuadPart << std::endl;
            lsa->out << "MinPasswordAge       : " << DomainPasswordInfo.MinPasswordAge.QuadPart << std::endl;
            LsaFreeReturnBuffer(response);
        }
        return result;
    }

    bool Proxy::ClearCachedCredentials() const {
        CLEAR_CACHED_CREDENTIALS_REQUEST request;
        void* response;
        return CallPackage(request, &response);
    }

    bool Proxy::DecryptDpapiMasterKey() const {
        DECRYPT_DPAPI_MASTER_KEY_REQUEST request;
        lsa->out << "Size: " << sizeof(request) << std::endl;
        DECRYPT_DPAPI_MASTER_KEY_RESPONSE* response;
        auto result{ CallPackage(request, &response) };
        // Parse response
        return result;
    }

    bool Proxy::DeleteTbalSecrets() const {
        DELETE_TBAL_SECRETS_REQUEST request;
        void* response{ nullptr };
        return CallPackage(request, &response);
    }

    bool Proxy::DeriveCredential(PLUID luid, DeriveCredType type, const std::vector<byte>& mixingBits) const {
        size_t requestLength{ sizeof(DERIVECRED_REQUEST) + mixingBits.size() };
        auto request{ reinterpret_cast<DERIVECRED_REQUEST*>(std::malloc(requestLength)) };
        request->MessageType = PROTOCOL_MESSAGE_TYPE::DeriveCredential;
        request->LogonSession.LowPart = luid->LowPart;
        request->LogonSession.HighPart = luid->HighPart;
        request->DeriveCredType = static_cast<ULONG>(type);
        request->DeriveCredInfoLength = mixingBits.size();
        std::memcpy(request->DeriveCredSubmitBuffer, mixingBits.data(), mixingBits.size());
        DERIVECRED_RESPONSE* response;
        auto result{ CallPackage(request, requestLength, &response) };
        if (result) {
            std::string cred(reinterpret_cast<const char*>(&response->DeriveCredReturnBuffer), response->DeriveCredInfoLength);
            OutputHex(lsa->out, "Derived Cred", cred);
            LsaFreeReturnBuffer(response);
        }
        return result;
    }

    bool Proxy::EnumerateUsers() const {
        ENUMUSERS_REQUEST request;
        ENUMUSERS_RESPONSE* response;
        auto result{ CallPackage(request, &response) };
        if (result) {
            auto count{ response->NumberOfLoggedOnUsers };
            lsa->out << "NumberOfLoggedOnUsers: " << count << std::endl;
            lsa->out << "LogonIds             : ";
            for (size_t index{ 0 }; index < count; index++) {
                lsa->out << "0x" << reinterpret_cast<LARGE_INTEGER*>(response->LogonSessions)[index].QuadPart << ((index < (count - 1)) ? ", " : "");
            }
            lsa->out << std::endl
                     << "EnumHandles          : ";
            for (size_t index{ 0 }; index < count; index++) {
                lsa->out << "0x" << reinterpret_cast<ULONG*>(response->EnumHandles)[index] << ((index < (count - 1)) ? ", " : "");
            }
            lsa->out << std::endl;
            LsaFreeReturnBuffer(response);
        }
        return result;
    }

    bool Proxy::GenericPassthrough(const std::wstring& domainName, const std::wstring& packageName, std::vector<byte>& data) const {
        std::vector<byte> requestBytes(sizeof(PASSTHROUGH_REQUEST) + (domainName.size() + packageName.size()) * sizeof(wchar_t) + data.size(), 0);
        auto request{ reinterpret_cast<PPASSTHROUGH_REQUEST>(requestBytes.data()) };
        request->MessageType = PROTOCOL_MESSAGE_TYPE::GenericPassthrough;
        request->DomainName.Length = domainName.size() * sizeof(wchar_t);
        request->DomainName.MaximumLength = request->DomainName.Length;
        auto buffer{ reinterpret_cast<wchar_t*>(request + 1) };
        std::memcpy(buffer, domainName.data(), domainName.length() * sizeof(wchar_t));
        request->DomainName.Buffer = buffer;
        request->PackageName.Length = packageName.size() * sizeof(wchar_t);
        request->PackageName.MaximumLength = request->PackageName.Length;
        buffer = buffer + domainName.size();
        std::memcpy(buffer, packageName.data(), packageName.length() * sizeof(wchar_t));
        request->PackageName.Buffer = buffer;
        request->DataLength = data.size();
        buffer = buffer + packageName.size();
        std::memcpy(buffer, data.data(), data.size());
        request->LogonData = reinterpret_cast<PUCHAR>(buffer);
        PASSTHROUGH_RESPONSE* response;
        auto result{ CallPackage(*request, &response) };
        if (result) {
            data.resize(sizeof(response) + response->DataLength);
            std::memcpy(data.data(), response, sizeof(response) + response->DataLength);
            LsaFreeReturnBuffer(response);
        }
        return result;
    }

    bool Proxy::GetCredentialKey(PLUID luid) const {
        GET_CREDENTIAL_KEY_REQUEST request;
        request.LogonSession.LowPart = luid->LowPart;
        request.LogonSession.HighPart = luid->HighPart;
        GET_CREDENTIAL_KEY_RESPONSE* response;
        auto result{ CallPackage(request, &response) };
        if (result) {
            std::string shaOwf(reinterpret_cast<const char*>(&response->CredentialData), MSV1_0_SHA_PASSWORD_LENGTH);
            OutputHex(lsa->out, "ShaOwf", shaOwf);
            // If there is data past the length for the ShaOwf and the NtOwf, then the NtOwf offset will actually be the Dpapi key
            if (*reinterpret_cast<DWORD*>(&response->CredentialData[MSV1_0_SHA_PASSWORD_LENGTH + MSV1_0_OWF_PASSWORD_LENGTH])) {
                std::string dpapiKey(reinterpret_cast<const char*>(&response->CredentialData[MSV1_0_SHA_PASSWORD_LENGTH]), MSV1_0_CREDENTIAL_KEY_LENGTH);
                OutputHex(lsa->out, "DpapiKey", dpapiKey);
            } else {
                std::string ntOwf(reinterpret_cast<const char*>(&response->CredentialData[MSV1_0_SHA_PASSWORD_LENGTH]), MSV1_0_OWF_PASSWORD_LENGTH);
                OutputHex(lsa->out, "NtOwf", ntOwf);
            }
            LsaFreeReturnBuffer(response);
        }
        return result;
    }

    bool Proxy::GetStrongCredentialKey() const {
        GET_STRONG_CREDENTIAL_KEY_REQUEST request;
        GET_STRONG_CREDENTIAL_KEY_RESPONSE* response;
        auto result{ CallPackage(request, &response) };
        // Parse response
        return result;
    }

    bool Proxy::GetUserInfo(PLUID luid) const {
        GETUSERINFO_REQUEST request;
        request.LogonSession.LowPart = luid->LowPart;
        request.LogonSession.HighPart = luid->HighPart;
        GETUSERINFO_RESPONSE* response;
        auto result{ CallPackage(request, &response) };
        if (result) {
            lsa->out << "LogonType      : " << response->LogonType << std::endl;
            auto offset{ reinterpret_cast<byte*>(response + 1) };
            auto sidLength{ reinterpret_cast<byte*>(response->UserName.Buffer) - offset };
            UNICODE_STRING sidString;
            if (RtlConvertSidToUnicodeString(&sidString, offset, true) == STATUS_SUCCESS) {
                std::wcout << L"UserSid        : " << sidString.Buffer << std::endl;
                RtlFreeUnicodeString(&sidString);
            }
            offset = offset + sidLength;
            std::wcout << L"UserName       : " << response->UserName.Buffer << std::endl;
            offset = offset + response->UserName.Length;
            std::wcout << L"LogonDomainName: " << response->LogonDomainName.Buffer << std::endl;
            offset = offset + response->LogonServer.Length;
            std::wcout << L"LogonServer    : " << response->LogonServer.Buffer << std::endl;
            LsaFreeReturnBuffer(response);
        }
        return result;
    }

    bool Proxy::Lm20ChallengeRequest() const {
        LM20_CHALLENGE_REQUEST request;
        LM20_CHALLENGE_RESPONSE* response;
        bool result{ CallPackage(request, &response) };
        if (result) {
            std::string challenge(reinterpret_cast<const char*>(&response->ChallengeToClient), sizeof(response->ChallengeToClient));
            OutputHex(lsa->out, "Challenge To Client", challenge);
            LsaFreeReturnBuffer(response);
        }
        return result;
    }

    bool Proxy::ProvisionTbal(PLUID luid) const {
        PROVISION_TBAL_REQUEST request;
        request.LogonSession.LowPart = luid->LowPart;
        request.LogonSession.HighPart = luid->HighPart;
        void* response{ nullptr };
        return CallPackage(request, &response);
    }

    bool Proxy::SetProcessOption(ProcessOption options, bool disable) const {
        SETPROCESSOPTION_REQUEST request;
        request.ProcessOptions = static_cast<ULONG>(options);
        request.DisableOptions = disable;
        void* response;
        return CallPackage(request, &response);
    }

    bool Proxy::TransferCred(PLUID sourceLuid, PLUID destinationLuid) const {
        TRANSFER_CRED_REQUEST request;
        request.OriginLogonId.LowPart = sourceLuid->LowPart;
        request.OriginLogonId.HighPart = sourceLuid->HighPart;
        request.DestinationLogonId.LowPart = destinationLuid->LowPart;
        request.DestinationLogonId.HighPart = destinationLuid->HighPart;
        void* response;
        return CallPackage(request, &response);
    }

    bool Proxy::CallPackage(const std::string& submitBuffer, void** returnBuffer) const {
        if (lsa->Connected()) {
            return lsa->CallPackage(MSV1_0_PACKAGE_NAME, submitBuffer, returnBuffer);
        }
        return false;
    }

    template<typename _Request, typename _Response>
    bool Proxy::CallPackage(const _Request& submitBuffer, _Response** returnBuffer) const {
        std::string stringSubmitBuffer(reinterpret_cast<const char*>(&submitBuffer), sizeof(decltype(submitBuffer)));
        return CallPackage(stringSubmitBuffer, reinterpret_cast<void**>(returnBuffer));
    }

    template<typename _Request, typename _Response>
    bool Proxy::CallPackage(_Request* submitBuffer, size_t submitBufferLength, _Response** returnBuffer) const {
        std::string stringSubmitBuffer(reinterpret_cast<const char*>(submitBuffer), submitBufferLength);
        return CallPackage(stringSubmitBuffer, reinterpret_cast<void**>(returnBuffer));
    }

    namespace Cache {
        std::unique_ptr<Netlogon::INTERACTIVE_INFO> GetLogonInfo(const std::wstring& domainName, const std::wstring& userName, std::wstring& computerName, const std::vector<byte>& hash, ULONG logonType) {
            auto logonInfo{ std::make_unique<Netlogon::INTERACTIVE_INFO>() };
            std::memset(logonInfo.get(), 0, sizeof(Netlogon::INTERACTIVE_INFO));
            // Populate the Identity portion of logonInfo
            auto& identity{ logonInfo->Identity };
            RtlInitUnicodeString(&identity.LogonDomainName, domainName.data());
            identity.ParameterControl = logonType;
            RtlInitUnicodeString(&identity.UserName, userName.data());
            if (!computerName.size()) {
                // For setting Workstation, there is no need to call GetComputerNameW twice to get the length
                // The value is the NetBIOS name which is at most 16 characters followed by a null terminator
                computerName = std::wstring(16 + 1, '\0');
                auto size{ static_cast<DWORD>(computerName.size()) };
                GetComputerNameW(computerName.data(), &size); // Assume this works for brevity
            }
            RtlInitUnicodeString(&identity.Workstation, computerName.data());
            // Populate the remainder of logonInfo
            std::memcpy(&logonInfo->NtOwfPassword, hash.data(), hash.size());
            return logonInfo;
        }

        std::vector<byte> GetSupplementalMitCreds(const std::wstring& domainName, const std::wstring& upn) {
            std::vector<byte> supplementalCreds((2 * sizeof(UNICODE_STRING)) + RoundUp(upn.length() * sizeof(wchar_t), sizeof(LONG)) + RoundUp(domainName.length() * sizeof(wchar_t), sizeof(LONG)), 0);
            // Add a unicode string for the UPN and immediately follow it with the UPN data
            UNICODE_STRING unicodeString;
            unicodeString.Length = upn.length();
            unicodeString.MaximumLength = upn.length();
            auto dataPtr{ reinterpret_cast<byte*>(supplementalCreds.data()) + sizeof(UNICODE_STRING) };
            if (unicodeString.Length > 0) {
                std::memcpy(dataPtr, upn.data(), upn.length() * sizeof(wchar_t));
                unicodeString.Buffer = (PWSTR)(reinterpret_cast<byte*>(supplementalCreds.data()) - reinterpret_cast<byte*>(dataPtr));
            } else {
                unicodeString.Buffer = nullptr;
            }
            std::memcpy(supplementalCreds.data(), &unicodeString, sizeof(UNICODE_STRING));
            // Add a unicode string for the domain name and immediately follow it with the domain name data
            dataPtr += RoundUp(unicodeString.Length * sizeof(wchar_t), sizeof(LONG)) + sizeof(UNICODE_STRING);
            unicodeString.Length = domainName.length();
            unicodeString.MaximumLength = domainName.length();
            if (unicodeString.Length > 0) {
                std::memcpy(dataPtr, domainName.data(), domainName.length() * sizeof(wchar_t));
                unicodeString.Buffer = (PWSTR)(reinterpret_cast<byte*>(supplementalCreds.data()) - reinterpret_cast<byte*>(dataPtr));
            } else {
                unicodeString.Buffer = nullptr;
            }
            std::memcpy(dataPtr - sizeof(UNICODE_STRING), &unicodeString, sizeof(UNICODE_STRING));
            return supplementalCreds;
        }

        // Populate the validation info to pass to MSV1_0
        // MSV1_0 supports both INFO2 and INFO4
        // We use INFO4 because it may store a UPN if needed
        std::unique_ptr<Netlogon::VALIDATION_SAM_INFO4> GetValidationInfo(Netlogon::PVALIDATION_SAM_INFO3 validationInfo, std::wstring* dnsDomainName) {
            auto validationInfoToUse{ std::make_unique<Netlogon::VALIDATION_SAM_INFO4>() };
            std::memset(validationInfoToUse.get(), 0, sizeof(Netlogon::VALIDATION_SAM_INFO4));
            std::memcpy(&validationInfoToUse, validationInfo, sizeof(Netlogon::VALIDATION_SAM_INFO2));
            // Add any resource groups that exist in the input validation structure
            // They will be stored in ExtraSids because INFO4 does not support them
            if (validationInfo->UserFlags & LOGON_RESOURCE_GROUPS) {
                if (validationInfo->ResourceGroupCount != 0) {
                    auto newGroupCount{ validationInfo->SidCount + validationInfo->ResourceGroupCount };
                    auto newGroups{ reinterpret_cast<Netlogon::PSID_AND_ATTRIBUTES>(std::malloc(sizeof(Netlogon::SID_AND_ATTRIBUTES) * newGroupCount)) }; // Assume this works for brevity
                    std::memcpy(newGroups, validationInfo->ExtraSids, validationInfo->SidCount * sizeof(Netlogon::SID_AND_ATTRIBUTES));
                    auto sidCount{ validationInfo->SidCount };
                    for (auto index{ 0 }; index < validationInfo->ResourceGroupCount; index++) {
                        newGroups[sidCount + index].Attributes = validationInfo->ResourceGroupIds[index].Attributes;
                        newGroups[sidCount + index].Sid = MakeDomainRelativeSid(validationInfo->ResourceGroupDomainSid, validationInfo->ResourceGroupIds[index].RelativeId); // Assume this works for brevity
                    }
                    Netlogon::VALIDATION_SAM_INFO2 info2;
                    info2.UserFlags |= LOGON_EXTRA_SIDS;
                    info2.SidCount = newGroupCount;
                    info2.ExtraSids = newGroups;
                    std::memcpy(&validationInfoToUse, &info2, sizeof(Netlogon::VALIDATION_SAM_INFO2));
                }
            }
            if (dnsDomainName) {
                RtlInitUnicodeString(&validationInfoToUse->DnsLogonDomainName, dnsDomainName->data());
            }
            return validationInfoToUse;
        }
    }
}