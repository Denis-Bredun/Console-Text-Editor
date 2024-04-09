#include <iostream>
#include <fstream>
#include <filesystem>
#include <stack>
#include <map>
#include <functional>
#include <windows.h>

enum class TypesOfCommands {
	Copy,
	Paste,
	Cut,
	Delete,
	Undo,
	Redo
};

class centerAlign {
private:
	std::string text;
	static CONSOLE_SCREEN_BUFFER_INFO csbi;
	static int coefficientConsideringTheSizeOfText, widthOfTheWindow, padding;

public:
	explicit centerAlign(const std::string& str) : text(str) { 
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi); 
		coefficientConsideringTheSizeOfText = 10;
		widthOfTheWindow = csbi.srWindow.Right - csbi.srWindow.Left + coefficientConsideringTheSizeOfText;
	}

	friend std::ostream& operator<<(std::ostream& os, const centerAlign& cs) {
		padding = (widthOfTheWindow - cs.text.size()) / 2;
		os << std::string(padding, ' ') << cs.text;
		return os;
	}
};

CONSOLE_SCREEN_BUFFER_INFO centerAlign::csbi;
int centerAlign::coefficientConsideringTheSizeOfText, centerAlign::widthOfTheWindow, centerAlign::padding;

class Notification {
private:
	static HANDLE consoleHandle;
	static int codeOfRed, codeOfGreen;

	static void changeConsoleColor(int colorCode = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) {
		SetConsoleTextAttribute(consoleHandle, colorCode);
		codeOfGreen = 10;
		codeOfRed = 12;
	}

	static void printTextWithSpecificColor(int colorCode, std::string text) {
		changeConsoleColor(colorCode);
		std::cout << "\n" << centerAlign(text + "\n\n");
		changeConsoleColor();
	}

public:
	static void setStdHandle() {
		consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	static void successNotification(std::string text) {
		printTextWithSpecificColor(codeOfGreen, "Успіх: " + text);
		system("pause");
		system("cls");
	}

	static void errorNotification(std::string text, bool shallConsoleBeCleaned = true) {
		printTextWithSpecificColor(codeOfRed, "Помилка: " + text);
		system("pause");
		if(shallConsoleBeCleaned)
			system("cls");
	}
};

HANDLE Notification::consoleHandle;
int Notification::codeOfRed, Notification::codeOfGreen;

class Clipboard {
private:
	std::stack<std::string> clipboard;

public:
	void addData(std::string data) {
		clipboard.push(data);
	}

	std::string getDataByIndex(int index) {
		return clipboard._Get_container()[index];
	}

	int size() {
		return clipboard.size();
	}

	bool isEmpty() {
		return clipboard.size() == 0;
	}

	void printClipboard() {
		system("cls");
		for (int i = 0; i < clipboard.size(); i++)
			std::cout << "\n" << i + 1 << ") " << getDataByIndex(i);
		std::cout << std::endl;
	}
};

class Session;
class SessionsHistory;

class Editor {
private:
	friend class Program;

	static SessionsHistory* sessionsHistory;
	static Session* currentSession;
	static std::string* currentText;

	void tryToLoadSessions();
	void tryToUnloadSessions();

public:
	Editor();

	~Editor() {
		delete sessionsHistory;
		if(currentText)
			delete (currentText);
	}

	void copy(std::string textToProcess, int startPosition, int endPosition);
	void paste(std::string* textToProcess, int startPosition, int endPosition, std::string textToPaste);
	void cut(std::string* textToProcess, int startPosition, int endPosition);
	void remove(std::string* textToProcess, int startPosition, int endPosition);

	static Session* getCurrentSession();
	static std::string* getCurrentText();

	static void printCurrentText();
};

class Command {
protected:
	Editor* editor;
	int startPosition, endPosition;
	std::string textToProcess, textToPaste;
	Command* previousCommand, *commandToUndoOrRedo;

public:
	virtual void execute() = 0;
	virtual void undo() = 0;

	void setParameters(TypesOfCommands typeOfCommand, Command* previousCommand, Command* commandToUndoOrRedo, int startPosition, int endPosition, std::string textToPaste) {
		if (typeOfCommand == TypesOfCommands::Undo || typeOfCommand == TypesOfCommands::Redo)
		{
			this->commandToUndoOrRedo = commandToUndoOrRedo;
			return;
		}

		if (startPosition > endPosition)
			std::swap(startPosition, endPosition);

		this->startPosition = startPosition;
		this->endPosition = endPosition;
		this->textToProcess = *(Editor::getCurrentText());
		this->previousCommand = previousCommand;

		if (typeOfCommand == TypesOfCommands::Paste)
			this->textToPaste = textToPaste;
	}

	std::string getTextToProcess() {
		return textToProcess;
	}

	std::string getTextToPaste() {
		return textToPaste;
	}

	int getStartPosition() {
		return startPosition;
	}

	int getEndPosition() {
		return endPosition;
	}

	void setTextToProcess(std::string textToProcess) {
		this->textToProcess = textToProcess;
	}

	void setTextToPaste(std::string textToPaste) {
		this->textToPaste = textToPaste;
	}

	void setStartPosition(int startPosition) {
		this->startPosition = startPosition;
	}

	void setEndPosition(int endPosition) {
		this->endPosition = endPosition;
	}

	void setPreviousCommand(Command* previousCommand) {
		this->previousCommand = previousCommand;
	}
};

class Session {
private:
	int currentCommandIndexInHistory;
	std::stack<Command*, std::vector<Command*>> commandsHistory;
	Clipboard* clipboard;
	std::string name;

public:
	Session() { 
		clipboard = new Clipboard(); 
		currentCommandIndexInHistory = -1;
	}

	Session(std::string filename) {
		clipboard = new Clipboard();
		currentCommandIndexInHistory = -1;
		name = filename;
	}

	~Session() {
		while (!commandsHistory.empty()) {
			if (commandsHistory.top())
				delete commandsHistory.top();
			commandsHistory.pop();
		}
		delete clipboard;
	}

	std::string getName() {
		return name;
	}

	bool setName(std::string filename) {
		std::string forbiddenCharacters = "/\\\":?*|<>";

		for (char ch : forbiddenCharacters)
			if (filename.find(ch) != std::string::npos)
				return false;

		name = filename + ".txt";

		return true;
	}

	void addCommandAsLast(Command* command) {
		commandsHistory.push(command);
	}

	void deleteLastCommand() {
		delete commandsHistory.top();
		commandsHistory.pop();
	}

	void deleteCommandByIndex(int index) {
		auto ptrOnUnderlyingContainer = &commandsHistory._Get_container();
		auto ptrOnRetiringCommand = (*ptrOnUnderlyingContainer)[index];

		std::vector<Command*> notRetiringElements;
		remove_copy(ptrOnUnderlyingContainer->begin(), ptrOnUnderlyingContainer->end(),
			back_inserter(notRetiringElements), ptrOnRetiringCommand);

		commandsHistory = std::stack<Command*, std::vector<Command*>>(notRetiringElements);

		delete ptrOnRetiringCommand;
	}

	Command* getLastCommand() {
		return commandsHistory.top();
	}

	Command* getCommandByIndex(int index) {
		return commandsHistory._Get_container()[index];
	}

	bool isCommandsHistoryEmpty() {
		return commandsHistory.size() == 0;
	}

	int sizeOfCommandsHistory() {
		return commandsHistory.size();
	}

	Clipboard* getClipboard() {
		return clipboard;
	}

	int getCurrentCommandIndexInHistory() {
		return currentCommandIndexInHistory;
	}

	void setCurrentCommandIndexInHistory(int currentCommandIndexInHistory) {
		this->currentCommandIndexInHistory = currentCommandIndexInHistory;
	}
};

class SessionsHistory {
private:
	std::stack<Session*, std::vector<Session*>> sessions;

public:
	~SessionsHistory() {
		while (!sessions.empty()) {
			delete sessions.top();
			sessions.pop();
		}
	}

	void addSessionToEnd(Session* session) {
		sessions.push(session);
	}

	Session* getLastSession() {
		return sessions.top();
	}

	Session* getFirstSession() {
		return sessions._Get_container().front();
	}

	Session* getSessionByIndex(int index) {
		return sessions._Get_container()[index];
	}

	std::string deleteLastSession() {
		std::string filename = sessions.top()->getName();

		delete sessions.top();
		sessions.pop();

		return filename;
	}

	std::string deleteFirstSession() {
		return deleteSessionByIndex(0);
	}

	std::string deleteSessionByIndex(int index) {
		auto ptrOnUnderlyingContainer = &sessions._Get_container();
		auto ptrOnRetiringSession = (*ptrOnUnderlyingContainer)[index];

		std::string filename = ptrOnRetiringSession->getName();

		std::vector<Session*> notRetiringElements;
		remove_copy(ptrOnUnderlyingContainer->begin(), ptrOnUnderlyingContainer->end(),
			back_inserter(notRetiringElements), ptrOnRetiringSession);

		sessions = std::stack<Session*, std::vector<Session*>>(notRetiringElements);

		delete ptrOnRetiringSession;

		return filename;
	}

	bool isEmpty() {
		return sessions.size() == 0;
	}

	int size() {
		return sessions.size();
	}

	void printSessionsHistory() {
		for (int i = 0; i < sessions.size(); i++)
			std::cout << "\n" << centerAlign("Сеанс #" + std::to_string(i + 1) + ": " + sessions._Get_container()[i]->getName());
		std::cout << std::endl;
	}
};

class CopyCommand : public Command {
public:
	CopyCommand(Editor* editor);

	void execute() override;

	void undo() override;
};

class DeleteCommand : public Command {
public:
	DeleteCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy();
};

class CutCommand : public Command {
public:
	CutCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy();
};

class PasteCommand : public Command {
public:
	PasteCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy();
};

class UndoCommand : public Command {
public:
	~UndoCommand() {
		if (commandToUndoOrRedo)
			delete (commandToUndoOrRedo);
	}

	void execute() override;

	void undo() override;
};

class RedoCommand : public Command {
public:
	~RedoCommand() {
		if (commandToUndoOrRedo)
			delete (commandToUndoOrRedo);
	}

	void execute() override;

	void undo() override;
};

class FilesManager {
private:
	friend class Editor;

	static const std::string METADATA_DIRECTORY,
		SESSIONS_METADATA_DIRECTORY,
		CLIPBOARD_METADATA_DIRECTORY,
		AVAILABLE_SESSIONS_FILE,
		AVAILABLE_SESSIONS_FULLPATH,
		SESSIONS_DIRECTORY;

	static std::stack<std::string> getFilepathsForMetadata(std::string directory) {
		std::stack<std::string> filesFromMetadataDirectory;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
			if (entry.path().extension() == ".txt")
				filesFromMetadataDirectory.push(entry.path().string());

		return filesFromMetadataDirectory;
	}

	static void deleteMetadataForDeletedSessions(SessionsHistory* sessionsHistory, std::string directory) {
		if (!std::filesystem::exists(directory))
			return;

		std::stack<std::string> filesFromMetadataDirectory = getFilepathsForMetadata(directory);

		if (sessionsHistory->isEmpty()) {
			while (!filesFromMetadataDirectory.empty()) {
				remove(filesFromMetadataDirectory.top().c_str());
				filesFromMetadataDirectory.pop();
			}
		}
		else {
			bool isPartOfRealtimeSessions = false;
			for (int i = 0; i < filesFromMetadataDirectory.size(); i++)
			{
				for (int j = 0; j < sessionsHistory->size(); j++)
				{
					if (filesFromMetadataDirectory._Get_container()[i] == sessionsHistory->getSessionByIndex(j)->getName())
						isPartOfRealtimeSessions = true;
				}
				if (!isPartOfRealtimeSessions)
					remove(filesFromMetadataDirectory._Get_container()[i].c_str());
				isPartOfRealtimeSessions = false;
			}
		}
	}

	static void writeSessionsMetadata(SessionsHistory* sessionsHistory) {
		deleteMetadataForDeletedSessions(sessionsHistory, SESSIONS_METADATA_DIRECTORY);

		std::string filename, typeOfCommand, delimiter = "---\n";
		Session* session;
		Command* command;

		if (!std::filesystem::exists(SESSIONS_METADATA_DIRECTORY))
			std::filesystem::create_directories(SESSIONS_METADATA_DIRECTORY);

		std::ofstream ofs_aval_session(AVAILABLE_SESSIONS_FULLPATH);

		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			session = sessionsHistory->getSessionByIndex(i);

			filename = session->getName();
			ofs_aval_session << filename << std::endl;

			std::ofstream ofs_session(SESSIONS_METADATA_DIRECTORY + filename);

			ofs_session << session->getName() << std::endl;
			ofs_session << session->sizeOfCommandsHistory() << std::endl;
			ofs_session << session->getCurrentCommandIndexInHistory() << std::endl;

			for (int j = 0; j < session->sizeOfCommandsHistory(); j++)
			{
				command = session->getCommandByIndex(j);

				if (std::string(typeid(*command).name()) == "class PasteCommand")
					typeOfCommand = "PasteCommand\n";
				else if (std::string(typeid(*command).name()) == "class CutCommand")
					typeOfCommand = "CutCommand\n";
				else
					typeOfCommand = "DeleteCommand\n";

				ofs_session << typeOfCommand;

				ofs_session << delimiter;

				ofs_session << command->getTextToProcess() << std::endl;

				if (command->getTextToProcess() == "")
					ofs_session << std::endl;

				ofs_session << delimiter;

				if(typeOfCommand == "PasteCommand\n")

				{
					ofs_session << command->getTextToPaste() << std::endl;

					if (command->getTextToPaste() == "")
						ofs_session << std::endl;

					ofs_session << delimiter;
				}

				ofs_session << command->getStartPosition() << std::endl;

				ofs_session << command->getEndPosition() << std::endl;
			}

			ofs_session.close();
		}

		ofs_aval_session.close();
	}

	static void readSessionsMetadata(SessionsHistory* sessionsHistory, Editor* editor) {

		std::ifstream ifs_available_sessions(AVAILABLE_SESSIONS_FULLPATH);

		if (!ifs_available_sessions) return;

		std::string line, typeOfCommand, text, delimiter = "---";
		int countOfCommands;
		Session* session;
		Command* command, * previousCommand;

		std::stack<std::string> available_sessions;

		while (getline(ifs_available_sessions, line))
			available_sessions.push(line);

		ifs_available_sessions.close();

		for (int i = 0; i < available_sessions.size(); i++)
		{
			std::ifstream ifs_session(SESSIONS_METADATA_DIRECTORY + available_sessions._Get_container()[i]);

			getline(ifs_session, line);
			session = new Session(line);

			getline(ifs_session, line);
			countOfCommands = stoi(line);

			getline(ifs_session, line);
			session->setCurrentCommandIndexInHistory(stoi(line));

			for (int j = 0; j < countOfCommands; j++)
			{
				getline(ifs_session, typeOfCommand);
				if (typeOfCommand == "CutCommand")
					command = new CutCommand(editor);
				else if (typeOfCommand == "PasteCommand")
					command = new PasteCommand(editor);
				else
					command = new DeleteCommand(editor);

				if (session->sizeOfCommandsHistory() > 0)
					command->setPreviousCommand(session->getLastCommand());

				getline(ifs_session, line);
				while (getline(ifs_session, line)) {
					if (line == delimiter)
						break;
					text += line + "\n";
				}
				command->setTextToProcess(text);

				if(typeOfCommand == "PasteCommand")
				{
					text = "";
					while (getline(ifs_session, line)) {
						if (line == delimiter)
							break;
						text += line + "\n";
					}
					command->setTextToPaste(text);
				}

				getline(ifs_session, line);
				command->setStartPosition(stoi(line));

				getline(ifs_session, line);
				command->setEndPosition(stoi(line));

				session->addCommandAsLast(command);
				text = "";
			}

			sessionsHistory->addSessionToEnd(session);

			ifs_session.close();
		}
	}

	static void writeClipboardMetadata(SessionsHistory* sessionsHistory) {
		deleteMetadataForDeletedSessions(sessionsHistory, CLIPBOARD_METADATA_DIRECTORY); // отдельно сделать удаление данных для clipboard, или просто взглянуть на параметры...

		std::string delimiter = "---\n";
		Clipboard* clipboard;

		if (!std::filesystem::exists(CLIPBOARD_METADATA_DIRECTORY))
			std::filesystem::create_directories(CLIPBOARD_METADATA_DIRECTORY);


		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			std::ofstream ofs(CLIPBOARD_METADATA_DIRECTORY  + sessionsHistory->getSessionByIndex(i)->getName());

			clipboard = sessionsHistory->getSessionByIndex(i)->getClipboard();

			for (int j = 0; j < clipboard->size(); j++)
			{
				ofs << clipboard->getDataByIndex(j) << std::endl;
				ofs << delimiter;
			}

			ofs.close();
		}
	}

	static void readClipboardMetadata(SessionsHistory* sessionsHistory) {
		Clipboard* clipboard;
		std::string data, text;
		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			std::ifstream ifs(CLIPBOARD_METADATA_DIRECTORY  + sessionsHistory->getSessionByIndex(i)->getName());

			clipboard = sessionsHistory->getSessionByIndex(i)->getClipboard();
			while (getline(ifs, data))
			{
				if (data == "---")
				{
					text = text.substr(0, text.size() - 1);
					clipboard->addData(text);
					text = "";
				}
				else
					text += data + "\n";
			}

			ifs.close();
		}
	}

public:
	static std::string getSessionsDirectory() {
		return SESSIONS_DIRECTORY;
	}

	static std::string readFullDataFromFile(std::string fullFilepath) {
		std::string text, line;

		std::ifstream file(fullFilepath);

		while (getline(file, line))
			text += line + "\n";

		file.close();

		return text;
	}

	static bool overwriteDataInExistingFile(std::string fullFilepath, std::string newData) {
		std::ofstream file(fullFilepath);

		if (!file.is_open())
			return false;

		file << newData;
		file.close();

		return true;
	}
};

const std::string FilesManager::METADATA_DIRECTORY = "Metadata\\",
FilesManager::SESSIONS_METADATA_DIRECTORY = METADATA_DIRECTORY  + "Sessions\\",
FilesManager::CLIPBOARD_METADATA_DIRECTORY = METADATA_DIRECTORY  + "Clipboard\\",
FilesManager::AVAILABLE_SESSIONS_FILE = "Available_Sessions.txt",
FilesManager::AVAILABLE_SESSIONS_FULLPATH = METADATA_DIRECTORY + AVAILABLE_SESSIONS_FILE,
FilesManager::SESSIONS_DIRECTORY = "Sessions\\";

void Editor::tryToLoadSessions() {
	FilesManager::readSessionsMetadata(sessionsHistory, this);
	FilesManager::readClipboardMetadata(sessionsHistory);
}

void Editor::tryToUnloadSessions() {
	FilesManager::writeClipboardMetadata(sessionsHistory);
	FilesManager::writeSessionsMetadata(sessionsHistory);
}

Editor::Editor() {
	this->sessionsHistory = new SessionsHistory();
}

void Editor::copy(std::string textToProcess, int startPosition, int endPosition) {
	this->currentSession->getClipboard()->addData(textToProcess.substr(startPosition, endPosition - startPosition + 1));
}

void Editor::paste(std::string* textToProcess, int startPosition, int endPosition, std::string textToPaste) {
	if (startPosition == endPosition)
	{
		if(startPosition + 1 == textToProcess->size())
		{
			*textToProcess += " ";
			startPosition++;
		}
		(*textToProcess).insert(startPosition, textToPaste);
	}
	else
		(*textToProcess).replace(startPosition, endPosition - startPosition + 1, textToPaste);
	*currentText = *textToProcess;
}

void Editor::cut(std::string* textToProcess, int startPosition, int endPosition) {
	copy(*textToProcess, startPosition, endPosition);
	remove(textToProcess, startPosition, endPosition);
	*currentText = *textToProcess;
}

void Editor::remove(std::string* textToProcess, int startPosition, int endPosition) {
	(*textToProcess).erase(startPosition, endPosition - startPosition + 1);
	*currentText = *textToProcess;
}

Session* Editor::getCurrentSession() {
	return currentSession;
}

std::string* Editor::getCurrentText() {
	return currentText;
}

void Editor::printCurrentText() {
	system("cls");
	std::cout << "\n" << centerAlign("Зміст файлу " + currentSession->getName() + ":\n");
	std::cout << *currentText << std::endl;
}

SessionsHistory* Editor::sessionsHistory;
Session* Editor::currentSession;
std::string* Editor::currentText;

CopyCommand::CopyCommand(Editor* editor) {
	this->editor = editor;
}

void CopyCommand::execute() {
	editor->copy(textToProcess, startPosition, endPosition);
}

void CopyCommand::undo() { }

Command* CopyCommand::copy() {
	return new CopyCommand(*this);
}

DeleteCommand::DeleteCommand(Editor* editor) {
	this->editor = editor;
}

void DeleteCommand::execute() {
	editor->remove(&textToProcess, startPosition, endPosition);
}

void DeleteCommand::undo() {
	*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
}

Command* DeleteCommand::copy() {
	return new DeleteCommand(*this);
}

CutCommand::CutCommand(Editor* editor) {
	this->editor = editor;
}

void CutCommand::execute() {
	editor->cut(&textToProcess, startPosition, endPosition);
}

void CutCommand::undo() {
	*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
}

Command* CutCommand::copy() {
	return new CutCommand(*this);
}

PasteCommand::PasteCommand(Editor* editor) {
	this->editor = editor;
}

void PasteCommand::execute() {
	editor->paste(&textToProcess, startPosition, endPosition, textToPaste);
}

void PasteCommand::undo() {
	if (previousCommand)
		*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
	else
		*(Editor::getCurrentText()) = "";
}

Command* PasteCommand::copy() {
	return new PasteCommand(*this);
}

void UndoCommand::execute() {
	commandToUndoOrRedo->undo();
}

void UndoCommand::undo() { }

Command* UndoCommand::copy() { return nullptr; }

void RedoCommand::execute() {
	*(Editor::getCurrentText()) = commandToUndoOrRedo->getTextToProcess();
}

void RedoCommand::undo() { }

Command* RedoCommand::copy() { return nullptr; }

class CommandsManager {
private:
	std::map<TypesOfCommands, Command*> manager;

	void setParametersForCommand(TypesOfCommands typeOfCommand, int startPosition = 0, int endPosition = 0, std::string textToPaste = "") {
		Command* commandToUndoOrRedo = nullptr, * previousCommand = nullptr;

		if (typeOfCommand == TypesOfCommands::Undo)
			commandToUndoOrRedo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurrentCommandIndexInHistory());

		if(typeOfCommand == TypesOfCommands::Redo)
			commandToUndoOrRedo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurrentCommandIndexInHistory() + 1);

		if (Editor::getCurrentSession()->getCurrentCommandIndexInHistory() != -1 && typeOfCommand != TypesOfCommands::Undo && typeOfCommand != TypesOfCommands::Redo)
			previousCommand = Editor::getCurrentSession()->getLastCommand();

		manager[typeOfCommand]->setParameters(typeOfCommand, previousCommand, commandToUndoOrRedo, startPosition, endPosition, textToPaste);
	}

	void deleteUselessLeftCommandsIfNecessary(TypesOfCommands typeOfCommand) {
		if (typeOfCommand != TypesOfCommands::Undo && typeOfCommand != TypesOfCommands::Redo)
			if (isThereAnyCommandForward()) {
				int countOfCommandsToDelete = Editor::getCurrentSession()->sizeOfCommandsHistory() - 1 - Editor::getCurrentSession()->getCurrentCommandIndexInHistory();
				for (int i = 1; i <= countOfCommandsToDelete; i++)
					Editor::getCurrentSession()->deleteLastCommand();
			}
	}

public:
	CommandsManager(Editor* editor) {
		manager[TypesOfCommands::Copy] = new CopyCommand(editor);
		manager[TypesOfCommands::Paste] = new PasteCommand(editor);
		manager[TypesOfCommands::Cut] = new CutCommand(editor);
		manager[TypesOfCommands::Delete] = new DeleteCommand(editor);
		manager[TypesOfCommands::Undo] = new UndoCommand();
		manager[TypesOfCommands::Redo] = new RedoCommand();
	}

	~CommandsManager() {
		for (auto& pair : manager) {
			delete (pair.second);
		}
		manager.clear();
	}

	bool isThereAnyCommandForward() {
		return !Editor::getCurrentSession()->isCommandsHistoryEmpty() &&
			Editor::getCurrentSession()->getCurrentCommandIndexInHistory() < Editor::getCurrentSession()->sizeOfCommandsHistory() - 1;
	}

	void invokeCommand(TypesOfCommands typeOfCommand, int startPosition = 0, int endPosition = 0, std::string textToPaste = "") {

		deleteUselessLeftCommandsIfNecessary(typeOfCommand);
		setParametersForCommand(typeOfCommand, startPosition, endPosition, textToPaste);

		manager[typeOfCommand]->execute();

		if (typeOfCommand != TypesOfCommands::Undo && typeOfCommand != TypesOfCommands::Redo && typeOfCommand != TypesOfCommands::Copy)
			Editor::getCurrentSession()->addCommandAsLast(manager[typeOfCommand]->copy());

		if (typeOfCommand != TypesOfCommands::Undo)
		{
			if (typeOfCommand != TypesOfCommands::Copy)
				Editor::getCurrentSession()->setCurrentCommandIndexInHistory(Editor::getCurrentSession()->getCurrentCommandIndexInHistory() + 1);
		}
		else
			Editor::getCurrentSession()->setCurrentCommandIndexInHistory(Editor::getCurrentSession()->getCurrentCommandIndexInHistory() - 1);
	}
};

class Program {
private:
	Editor* editor;
	CommandsManager* commandsManager;

	bool validateEnteredNumber(std::string option, short firstOption, short lastOption) {
		if (option.empty())
			return false;

		for (char num : option)
			if (num < '0' || num > '9')
				return false;

		auto convertedValue = std::stoull(option);

		return firstOption <= convertedValue && convertedValue <= lastOption;
	}

	int enterNumberInRange(std::string message, int firstOption, int lastOption, bool shallConsoleBeCleaned = true, std::string error = "були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!") {
		bool isOptionVerified = false;
		std::string option;

		std::cout << "\n" << centerAlign(message);
		getline(std::cin, option);

		isOptionVerified = validateEnteredNumber(option, firstOption, lastOption);

		if (!isOptionVerified)
			Notification::errorNotification(error, shallConsoleBeCleaned);

		return isOptionVerified? stoi(option) : -1;
	}

	void wayToGetTextForAddingMenu(int& choice) {
		std::cout << "\n" << centerAlign("Як ви хочете додати текст:\n");
		std::cout << centerAlign("0. Назад\n");
		std::cout << centerAlign("1. Ввівши з клавіатури\n");
		std::cout << centerAlign("2. З буферу обміну\n");
		choice = enterNumberInRange("Ваш вибір: ", 0, 2);
	}

	std::string getTextUsingKeyboard() {
		std::string line, text;

		std::cout << "\n" << centerAlign("Введіть текст (зупинити - з наступного рядка введіть -1):\n");
		while (getline(std::cin, line)) {
			if (line == "-1")
				break;
			else if (line == "")
				text += "\n";
			else
				text += line;
		}

		return text;
	}

	void getTextFromClipboardMenu(int& choice) {
		std::cout << "\n" << centerAlign("Які дані бажаєте отримати з буферу обміну:\n");
		std::cout << centerAlign("0. Назад\n");
		std::cout << centerAlign("1. Останні\n");
		std::cout << centerAlign("2. Найперші\n");
		std::cout << centerAlign("3. За індексом\n");
		choice = enterNumberInRange("Ваш вибір: ", 0, 3);
	}

	std::string getTextFromClipboard() {
		auto clipboard = Editor::currentSession->getClipboard();
		if (clipboard->isEmpty()) {
			Notification::errorNotification("в буфері обміну ще немає даних!");
			return "";
		}

		int choice;

		clipboard->printClipboard();
		getTextFromClipboardMenu(choice);

		switch (choice)
		{
		case 0:
			std::cout << "\n" << centerAlign("Повернення до меню вибору способа додавання текста.\n\n");
			system("pause");
			system("cls");
			return "";
		case 1:
			return clipboard->getDataByIndex(clipboard->size() - 1);
		case 2:
			return clipboard->getDataByIndex(0);
		case 3:
			choice = enterNumberInRange("Введіть номер даних: ", 1, clipboard->size());
			if(choice != -1)
				return clipboard->getDataByIndex(choice - 1);
			return "";
		default:
			return "";
		}
	}

	bool executeGettingTextForAdding(std::string& text) {
		int choice;

		do
		{
			Editor::printCurrentText();
			wayToGetTextForAddingMenu(choice);

			switch (choice)
			{
			case 0:
				std::cout << "\n" << centerAlign("Повернення до Меню дій над змістом.\n\n");
				system("pause");
				system("cls");
				return false;
			case 1:
				text = getTextUsingKeyboard();
				return true;
			case 2:
				text = getTextFromClipboard();
				return true;
			}
		} while (true);
	}

	void wayToPasteTextMenu(int& choice) {
		std::cout << "\n" << centerAlign("Як ви хочете вставити текст:\n");
		std::cout << centerAlign("0. Вийти в Меню дій над змістом\n");
		std::cout << centerAlign("1. В кінець\n");
		std::cout << centerAlign("2. На початок\n");
		std::cout << centerAlign("3. За індексом\n");
		std::cout << centerAlign("4. Замінивши певну частину в файлі\n");
		choice = enterNumberInRange("Ваш вибір: ", 0, 4);
	}

	void pasteText(int startIndex, int endIndex, std::string textToPaste) {
		commandsManager->invokeCommand(TypesOfCommands::Paste, startIndex, endIndex, textToPaste);
		Notification::successNotification("дані були успішно додані в файл!");
	}

	void pasteTextInBeginningOrEnd(int choice, std::string textToPaste) {
		if (Editor::currentText->size() == 0 || choice == 2)
			pasteText(0, 0, textToPaste);
		else if (choice == 1 && Editor::currentText->size() != 0)
			pasteText(Editor::currentText->size() - 1, Editor::currentText->size() - 1, textToPaste);
	}

	int pasteTextByIndex(std::string textToPaste) {
		int startOfRange = 0, endOfRange = 0;
		if (Editor::currentText->size() > 0)
			endOfRange = Editor::currentText->size() - 1;

		int index = enterNumberInRange("Введіть індекс, за яким будете вставляти текст: ", startOfRange, endOfRange);

		if (index == -1)
			return index;

		commandsManager->invokeCommand(TypesOfCommands::Paste, index, index, textToPaste);

		return index;
	}

	int pasteTextFromIndexToIndex(std::string textToPaste) {
		int startOfRange = 0, endOfRange = 0;
		if (Editor::currentText->size() > 0)
			endOfRange = Editor::currentText->size() - 1;

		int startIndex = enterNumberInRange("Початковий індекс: ", startOfRange, endOfRange, false, "був введений індекс, який виходить за межі тексту!");
		if (startIndex == -1) return -1;

		int endIndex = enterNumberInRange("Кінцевий індекс: ", startOfRange, endOfRange, true, "був введений індекс, який виходить за межі тексту!");
		if (endIndex == -1) return -1;

		commandsManager->invokeCommand(TypesOfCommands::Paste, startIndex, endIndex, textToPaste);

		return endIndex;
	}

	bool executeAddingTextToFile() {
		std::string textToPaste;
		bool shallTextBeAdded = executeGettingTextForAdding(textToPaste);
		if (!shallTextBeAdded || textToPaste == "")
			return false;

		int choice;
		Editor::printCurrentText();
		wayToPasteTextMenu(choice);

		switch (choice) {
		case 0:
			std::cout << "\n" << centerAlign("Повернення до Меню дій над змістом.\n\n");
			system("pause");
			system("cls");
			return false;
		case 1:
		case 2:
			pasteTextInBeginningOrEnd(choice, textToPaste);
			return true;
		case 3:
			choice = pasteTextByIndex(textToPaste);
			if(choice != -1)
				return true;
			return false;
		case 4:
			choice = pasteTextFromIndexToIndex(textToPaste);
			if (choice != -1)
				return true;
			return false;
		default:
			return false;
		}
	}

	void delCopyOrCutTextMenu(int& choice, std::string action) {
		std::cout << "\n" << centerAlign("Скільки хочете " + action + ":\n");
		std::cout << centerAlign("0. Назад\n");
		std::cout << centerAlign("1. Весь зміст\n");
		std::cout << centerAlign("2. З певного по певний індекс\n");
		choice = enterNumberInRange("Ваш вибір: ", 0, 2);
	}

	bool delCopyOrCutWholeText(TypesOfCommands typeOfCommand, std::string actionInPast) {
		commandsManager->invokeCommand(typeOfCommand, 0, Editor::currentText->size() - 1);
		Notification::successNotification("дані були успішно " + actionInPast + "!");
		return true;
	}

	bool delCopyOrCutFromIndexToIndex(TypesOfCommands typeOfCommand, std::string actionInPast) {
		int startIndex, endIndex;

		startIndex = enterNumberInRange("Початковий індекс: ", 0, Editor::currentText->size() - 1, true, "був введений індекс, який виходить за межі тексту!");
		if (startIndex == -1) return false;

		endIndex = enterNumberInRange("Кінцевий індекс: ", 0, Editor::currentText->size() - 1, true, "був введений індекс, який виходить за межі тексту!");
		if (endIndex == -1) return false;

		commandsManager->invokeCommand(typeOfCommand, startIndex, endIndex);
		Notification::successNotification("дані були успішно " + actionInPast + "!");
		return true;
	}

	bool delCopyOrCutText(TypesOfCommands typeOfCommand, std::string actionForMenu, std::string actionInPast) {
		int choice;

		Editor::printCurrentText();
		delCopyOrCutTextMenu(choice, actionForMenu);

		switch (choice)
		{
		case 0:
			std::cout << "\n" << centerAlign("Повернення до Меню дій над змістом.\n\n");
			system("pause");
			system("cls");
			return false;
		case 1:
			return delCopyOrCutWholeText(typeOfCommand, actionInPast);
		case 2:
			return delCopyOrCutFromIndexToIndex(typeOfCommand, actionInPast);
		}
	}

	bool chooseRootDelCopyOrCut(std::string action) {
		if (Editor::currentText->size() == 0)
		{
			Notification::errorNotification("немає тексту, який можна було б " + action + "!");
			return false;
		}
		else
		{
			system("cls");
			if (action == "видалити")
				return delCopyOrCutText(TypesOfCommands::Delete, action, "видалені");
			else if (action == "скопіювати")
				return delCopyOrCutText(TypesOfCommands::Copy, action, "скопійовані");
			else
				return delCopyOrCutText(TypesOfCommands::Cut, action, "вирізані");
		}
	}

	void makeActionsOnContentMenu(int& choice) {
		std::cout << "\n" << centerAlign("Меню дій над змістом:\n");
		std::cout << centerAlign("0. Назад\n");
		std::cout << centerAlign("1. Додати текст\n");
		std::cout << centerAlign("2. Видалити текст\n");
		std::cout << centerAlign("3. Копіювати текст\n");
		std::cout << centerAlign("4. Вирізати текст\n");
		std::cout << centerAlign("5. Скасувати команду\n");
		std::cout << centerAlign("6. Повторити команду\n");
		choice = enterNumberInRange("Ваш вибір: ", 0, 6);
	}

	void readDataFromFile() {
		Editor::currentText = new std::string(FilesManager::readFullDataFromFile(FilesManager::getSessionsDirectory() + Editor::currentSession->getName()));
	}

	bool undoAction() {
		if (Editor::currentSession->sizeOfCommandsHistory() > 0 && Editor::currentSession->getCurrentCommandIndexInHistory() != -1)
		{
			commandsManager->invokeCommand(TypesOfCommands::Undo);
			Notification::successNotification("команда була успішно скасована!");
		}
		else
			Notification::errorNotification("немає дій, які можна було б скасувати!");
		return Editor::currentSession->sizeOfCommandsHistory() > 1;
	}

	bool redoAction() {
		bool isThereAnyCommandForward = commandsManager->isThereAnyCommandForward();
		if (isThereAnyCommandForward)
		{
			commandsManager->invokeCommand(TypesOfCommands::Redo);
			Notification::successNotification("команда була успішно повторена!");

		}
		else
			Notification::errorNotification("немає дій, які можна було б повторити!");
		return isThereAnyCommandForward;
	}

	void executeMakeActionsOnContentMenu() {
		commandsManager = new CommandsManager(editor);
		bool wasTextSuccessfullyChanged;
		int choice;

		readDataFromFile();

		do
		{
			wasTextSuccessfullyChanged = false;
			Editor::printCurrentText();
			makeActionsOnContentMenu(choice);

			switch (choice)
			{
			case 0:
				std::cout << "\n" << centerAlign("Повернення до Меню для отримання сеансу.\n\n");
				system("pause");
				system("cls");
				delete (commandsManager);
				return;
			case 1:
				system("cls");
				wasTextSuccessfullyChanged = executeAddingTextToFile();
				break;
			case 2:
				wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("видалити");
				break;
			case 3:
				wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("скопіювати");
				break;
			case 4:
				wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("вирізати");
				break;
			case 5:
				wasTextSuccessfullyChanged = undoAction(); //комманду копирования не отменяем и не повторяем! Только то, что влияет на сам текст!
				break;
			case 6:
				wasTextSuccessfullyChanged = redoAction();
			}
			if(wasTextSuccessfullyChanged)
				FilesManager::overwriteDataInExistingFile(FilesManager::getSessionsDirectory() + Editor::currentSession->getName(), *(Editor::currentText));
		} while (true);
	}

	void templateForMenusAboutSessions(int& choice, std::string action) {
		std::cout << "\n" << centerAlign("Який сеанс хочете " + action + ":\n");
		std::cout << centerAlign("0. Назад\n");
		std::cout << centerAlign("1. Останній\n");
		std::cout << centerAlign("2. Найперший\n");
		std::cout << centerAlign("3. За позицією\n");
		choice = enterNumberInRange("Ваш вибір: ", 0, 3);
	}

	void templateForExecutingMenusAboutSessions(std::function<void(int&)> mainFunc, std::function<void(int&)> menu, std::function<void()> additionalFunc = nullptr) {
		int choice, index = -1;
		do
		{
			Editor::sessionsHistory->printSessionsHistory();
			menu(choice);
			switch (choice)
			{
			case 0:
				std::cout << "\n" << centerAlign("Повернення до Головного меню.\n\n");
				system("pause");
				system("cls");
				return;
			case 1:
			case 2:
			case 3:
				if (doesAnySessionExist())
				{
					index = choice == 1 ? Editor::sessionsHistory->size() : choice == 2 ? 1 : -1;
					mainFunc(index);

					if(index != -1 && additionalFunc)
					{
						system("cls");
						additionalFunc();
					}
				}
			}
		} while (true);
	}

	bool tryToEnterIndexForSession(int& index) {
		if (index == -1) {
			index = enterNumberInRange("Введіть номер сеансу: ", 1, Editor::sessionsHistory->size());
			if (index == -1)
				return false;
		}
		return true;
	}

	void printGettingSessionsMenu(int& choice) {
		templateForMenusAboutSessions(choice, "отримати");
	}

	void setCurrentSessionByIndex(int& index) {
		if(tryToEnterIndexForSession(index))
			Editor::currentSession = Editor::sessionsHistory->getSessionByIndex(index - 1);
	}

	void executeGettingSessionsMenu() {
		std::function<void(int&)> mainFunc = [this](int &index) {
			setCurrentSessionByIndex(index);
			};
		std::function<void(int&)> menu = [this](int& choice) {
			printGettingSessionsMenu(choice);
			};
		std::function<void()> additionalFunc = [this]() {
			executeMakeActionsOnContentMenu();
			};

		templateForExecutingMenusAboutSessions(mainFunc, menu, additionalFunc);
	}

	void printDeletingSessionsMenu(int& choice) {
		templateForMenusAboutSessions(choice, "видалити");
	}

	void deleteSessionByIndex(int index = -1) {
		if(tryToEnterIndexForSession(index)){
			std::string nameOfSession = Editor::sessionsHistory->deleteSessionByIndex(index - 1);
			std::string pathToSession = FilesManager::getSessionsDirectory() + nameOfSession;
			remove(pathToSession.c_str());

			Notification::successNotification("сеанс був успішно видалений!");
		}
	}

	void executeDeletingSessionsMenu() {
		std::function<void(int)> mainFunc = [this](int index) {
			deleteSessionByIndex(index);
			};
		std::function<void(int&)> menu = [this](int& choice) {
			printDeletingSessionsMenu(choice);
			};

		templateForExecutingMenusAboutSessions(mainFunc, menu);
	}

	void printManagingSessionsMenu(int& choice) {
		std::cout << centerAlign("Головне меню:\n");
		std::cout << centerAlign("0. Закрити програму\n");
		std::cout << centerAlign("1. Довідка\n");
		std::cout << centerAlign("2. Створити сеанс\n");
		std::cout << centerAlign("3. Відкрити сеанс\n");
		std::cout << centerAlign("4. Видалити сеанс\n");
		choice = enterNumberInRange("Ваш вибір: ", 0, 4);
	}

	void printReferenceInfo() {
		system("cls");
		std::cout << "\n" << centerAlign("Довідка до програми:\n\n");
		std::cout << centerAlign("Вашої уваги пропонується програму курсової роботи з об'єктно-орієнтованого програмування, побудована за допомогою патерну проектування \"Команда\".\n");
		std::cout << centerAlign("За допомогою програми можна створювати, редагувати та видаляти текстові файли, використовуючи при цьому команди Копіювання, Вставка, Вирізання та Відміна операції.\n\n");
		std::cout << centerAlign("Розробник: Бредун Денис Сергійович з групи ПЗ-21-1/9.\n\n");
		system("pause");
		system("cls");
	}

	bool doesSessionWithThisNameExists(std::string name) {
		for (int i = 0; i < editor->sessionsHistory->size(); i++)
			if (editor->sessionsHistory->getSessionByIndex(i)->getName() == name + ".txt")
				return true;

		return false;
	}

	void createSession() {
		std::string filename;

		std::cout << "\n" << centerAlign("Введіть ім'я сеансу (заборонені символи: /\\\":?*|<>): ");
		getline(std::cin, filename);

		Session* newSession = new Session();
		if (newSession->setName(filename))
		{
			if (doesSessionWithThisNameExists(filename)) {
				delete newSession;
				Notification::errorNotification("сеанс з таким іменем вже існує!");
				return;
			}

			if (!std::filesystem::exists(FilesManager::getSessionsDirectory()))
				std::filesystem::create_directories(FilesManager::getSessionsDirectory());

			std::string filepath = FilesManager::getSessionsDirectory() + newSession->getName();
			std::ofstream file(filepath);
			file.close();
			Editor::sessionsHistory->addSessionToEnd(newSession);
			Notification::successNotification("сеанс був успішно створений!");
		}
		else
		{
			delete newSession;
			Notification::errorNotification("були введені заборонені символи!");
		}
	}

	bool doesAnySessionExist() {
		if (Editor::sessionsHistory->isEmpty())
			Notification::errorNotification("в даний момент жодного сеансу немає!");
		return !Editor::sessionsHistory->isEmpty();
	}

	void setConsoleFont(int sizeOfFont) {
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(cfi);
		cfi.dwFontSize.Y = sizeOfFont;
		wcscpy_s(cfi.FaceName, L"Consolas");
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
	}

	void setConsoleFullScreenAndNonresized() {
		HWND consoleWindow = GetConsoleWindow();
		SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);

		system("mode con COLS=700");
		ShowWindow(consoleWindow, SW_MAXIMIZE);
		SendMessage(consoleWindow, WM_SYSKEYDOWN, VK_RETURN, 0x20000000);
	}

public:

	void setUp() {
		setConsoleFullScreenAndNonresized();
		setConsoleFont(22);
		Notification::setStdHandle();
	}

	void executeMainMenu() {
		int choice;
		editor = new Editor();
		editor->tryToLoadSessions(); 

		do
		{
			printManagingSessionsMenu(choice);
			switch (choice)
			{
			case 0:
				std::cout << "\n" << centerAlign("До побачення!\n");
				editor->tryToUnloadSessions(); //протестить!
				delete editor;
				exit(0);
			case 1:
				printReferenceInfo();
				continue;
			case 2:
				createSession();
				continue;
			case 3:
			case 4:
				if (doesAnySessionExist())
				{
					system("cls");
					choice == 3 ? executeGettingSessionsMenu() :
						executeDeletingSessionsMenu();
				}
			}

		} while (true);
	}
};

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	Program program;
	program.setUp();
	program.executeMainMenu();
}

//Заметки:

//виводити результат після кожної дії

//удалить лишнее

//удалить ненужные пункты в меню

//пользователь не должен вводить путь к файлам

//использовать относительный путь для ввода директорий

//путь по умолчанию

//я могу создавать файлы t1.ttxt

//если два файла имеют одинаковое имя, то сессия добавляется, а файл - нет

//доделать обработку истории команд и её откачку

//сделать, чтобы пользователь мог либо бесконечно отменять последнее действие, либо двигаться по истории комманд назад, ибо иначе записывать всю историю коммманд в файл и их данные - бессмысленно

//добавить команду обратную отмотке истории

//записывать в файл textToPaste

//запись с буфера обмена в файл - дерьмо

//при редактировании буфера обмена также надо учитывать команды Cut (но редактирование буфера обмена мы уберём)

//проверить запись и считывание метаданных нескольких файлов

//проверить все new на delete

//при вводе 0 происходит перемещение на предыдущий пункт меню, при этом об этом никак не уведомляется

//если в буфере обмена последние данные = тем, которые копируем в даннный момент, то не копируем их

//очищать консольное окно после операций?

//большое меню!

//выводить постоянно состояние файла и сессии!






//проверить код на лишние методы и классы, разделение логики по методам, короткие строки и понятный код, на лишние переменные и инструменты

//после разделения на файлы сделать using namespace std

//много system("cls");

//упорядочить методы по группам

//подумать о центрации текста

//много где используется enterNumberInRange, хотя там надо просто верификация введённого числа

//два метода changeFileMetadata имеют одинаковую логику

// (зупинити ввід - -1) - це потрібно ввести на наступному рядку

//проверить функции на правильное разделение по логике

//сделать многие меню поо структуре как executeMakeActionsOnContentMenu

//мб убрать метод отдельный для сохранения методанных буфера обмена?

//при вводе двух индексов при вставке или удалении разные реакции на ошибки

//отцентрировать меню?

//вынести отдельно методы из классов

//разное поведение, если вводить огромное полож число или отриц число при валидации

//при попытке удалить сессии и вводе:
/*
Ваш вибір: info

Помилка: були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!
*/

/*надо разделять валидацию вводимого числа и валидацию для меню*/

//убрать подобное: "class UndoCommand"

//норм ли:
//if (option == "exit")
//exit(1);
//			else if (option == "info")
//				return -1;
//норм ли case -1: в меню?

//убрать лищние библиотеки

//убрать 
//if (!doesAnySessionExist())
//continue; из множества мест

//вводить данные надо не в функции верификации ввода данных файла и меню!

//убрать длинные стрелки наподобие sessionsHistory->getSessionByIndex(i)->getClipboard()->

//мб разместить пункты менюх всех по функциям

//"параметри файлу", а не "системні"

//добавить заголовки как для страниц в пункты меню

//сделать многие методы в одном месте

//методы writeNewTextForContent и pasteDataFromClipboard имеют одинаковую логику

//классы команд наследуют ненужные вещи

//меню makeActionsOnContentMenu большое

//большие параметры передаются в методы

//ненужные заздалегідь оголошені класи - Command

//есть функции-болванки

//избавиться от лишних функций и классов

//фраза "Довідка:"

//вынести методы отдельно от классов

//меню действий над содержанием имеют одинаковые тектсты - можно вынести в отдельный метод












//доделать доп пункты из задания курсовой для меню

//возможно добавить предпросмотр при действиях над змістом

//обрабатывать закрытие пользователем проги, чтобы сохранять метаданные

//добавить историю действий для каждой сессии, как в гугл док

//идея вывода инфы с помощью html

//взять идеи у Стаса

//написать комменты к классам










//изменить лит джерела со "страниц" на "page"