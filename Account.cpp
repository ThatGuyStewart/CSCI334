#include "Account.h"

Account::Account(std::string email, std::string password)
	: m_email(std::move(email)), m_password(std::move(password))
{
}

std::string Account::getEmail() const
{
	return m_email;
}

bool Account::validatePassword(const std::string& password) const
{
	return m_password == password;
}

AdminAccount::AdminAccount(std::string email, std::string password)
	: Account(std::move(email), std::move(password))
{
}

bool AdminAccount::isAdmin() const
{
	return true;
}

UserAccount::UserAccount(std::string email, std::string password)
	: Account(std::move(email), std::move(password))
{
}

bool UserAccount::isAdmin() const
{
	return false;
}
