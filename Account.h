#pragma once
#include <string>

class Account
{
protected:
	std::string m_email;
	std::string m_password;
	Account(std::string email, std::string password);

public:
	virtual ~Account() = default;
	std::string getEmail() const;
	bool validatePassword(const std::string& password) const;
	virtual bool isAdmin() const = 0;
};

class AdminAccount : public Account
{
public:
	AdminAccount(std::string email, std::string password);
	bool isAdmin() const override;
};

class UserAccount : public Account
{
public:
	UserAccount(std::string email, std::string password);
	bool isAdmin() const override;
};

