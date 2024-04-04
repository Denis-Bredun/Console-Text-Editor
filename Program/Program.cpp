#include <iostream>
#include <stack>
#include <vector>
#include <regex>
#include <string>
#include <fstream>
#include <algorithm>
#include <functional>
#include <numeric>
#include <map>
#include <filesystem>
#include <windows.h>
#include <typeinfo>
using namespace std;

class Notification {
private:
	static void changeConsoleColor(int colorCode = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) {
		HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(consoleHandle, colorCode);
	}

	static void printTextWithSpecificColor(int colorCode, string text) {
		changeConsoleColor(colorCode);
		cout << "\n" << text << "\n\n";
		changeConsoleColor();
	}

public:
	static void successNotification(string text) {
		printTextWithSpecificColor(10, "Успіх: " + text);
	}

	static void errorNotification(string text) {
		printTextWithSpecificColor(12, "Помилка: " + text);
	}
};

class Clipboard {
private:
	stack<string> clipboard;
public:
	void addData(string data) {
		clipboard.push(data);
	}

	string getLastData() {
		return clipboard.top();
	}

	string getDataByIndex(int index) {
		return clipboard._Get_container()[index];
	}

	void deleteLastData() {
		clipboard.pop();
	}

	void deleteFirstData() {
		deleteDataByIndex(0);
	}

	void deleteDataByIndex(int index) {
		stack<string> newClipboard;

		for (int i = 0; i < clipboard.size(); i++)
		{
			if (i == index)
				continue;
			newClipboard.push(clipboard._Get_container()[i]);
		}
		clipboard = newClipboard;
	}

	bool isEmpty() {
		return clipboard.size() == 0;
	}

	int size() {
		return clipboard.size();
	}

	void printClipboard() {
		if (isEmpty()) {
			Notification::errorNotification("буфер обміну пустий!");
			return;
		}
		for (int i = 0; i < clipboard.size(); i++)
			cout << "\n" << i + 1 << ") " << clipboard._Get_container()[i];
		cout << endl;
	}
};

struct FileMetadata {
private:
	string filename, filepath;

	bool setDataIfValidInRegex(string& field, string data, string strTemplate) {
		smatch matches;
		regex regexTemplate(strTemplate);
		bool isDataValid = regex_search(data, matches, regexTemplate);
		if (isDataValid)
			field = data;
		return isDataValid;
	}

public:

	string getFilename() {
		return filename;
	}

	string getFilepath() {
		return filepath;
	}

	string getFullPath() {
		return filepath + "\\" + filename;
	}

	bool setFilename(string filename) {
		return setDataIfValidInRegex(this->filename, filename, "[^\\/:*?\"<>|]\.(txt|docx|doc)$");
	}

	bool setFilepath(string filepath) {
		return setDataIfValidInRegex(this->filepath, filepath, "^([A-Z]:)(\\\\[a-zA-ZА-Яа-яІіЇїЄєЁёЪъ_\\_\\-\\d\\s\\.]+)+$");
	}
};

class Editor;

enum class TypesOfCommands {
	Copy,
	Paste,
	Cut,
	Undo,
	Delete
};

class Command {
protected:
	friend class FilesManager;

	Editor* editor;
	int startPosition, endPosition;
	string textToProcess, lastStateOfText, textToPaste;
	string* ptrOnEditorsText;
	Command* command;
public:
	virtual void execute() = 0;
	virtual void undo() = 0;
	virtual Command* copy() = 0;

	void setEditor(Editor* editor) {
		this->editor = editor;
	}

	void setParameters(TypesOfCommands typeOfCommand, Command* command = nullptr, string* ptrOnEditorsText = nullptr, int startPosition = 0, int endPosition = 0, string textToPaste = "") {
		if (typeOfCommand == TypesOfCommands::Undo)
		{
			this->command = command;
			return;
		}

		if (startPosition > endPosition)
			swap(startPosition, endPosition);
		if (endPosition > (*ptrOnEditorsText).size() - 1)
			endPosition = (*ptrOnEditorsText).size() - 1;

		this->startPosition = startPosition;
		this->endPosition = endPosition;
		this->textToProcess = *ptrOnEditorsText;
		this->ptrOnEditorsText = ptrOnEditorsText;

		if (typeOfCommand == TypesOfCommands::Paste)
			this->textToPaste = textToPaste;
	}

	void setPtrOnEditorsText(string* ptrOnEditorsText) {
		this->ptrOnEditorsText = ptrOnEditorsText;
	}
};

class Session {
private:
	FileMetadata* fileMetadata;
	stack<Command*, vector<Command*>> commandsHistory;
	Clipboard* clipboard;

public:
	Session(FileMetadata* fileMetadata) {
		this->fileMetadata = fileMetadata;
		clipboard = new Clipboard();
	}

	~Session() {
		while (!commandsHistory.empty()) {
			if (commandsHistory.top())
				delete commandsHistory.top();
			commandsHistory.pop();
		}
		if (fileMetadata)
			delete fileMetadata;
		if (clipboard)
			delete clipboard;
	}

	string getName() {
		return fileMetadata->getFilename();
	}

	void changeName(string filename) {
		fileMetadata->setFilename(filename);
	}

	string getPath() {
		return fileMetadata->getFilepath();
	}

	void changePath(string filepath) {
		fileMetadata->setFilepath(filepath);
	}

	string getFullPath() {
		return fileMetadata->getFullPath();
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

		vector<Command*> notRetiringElements;
		remove_copy(ptrOnUnderlyingContainer->begin(), ptrOnUnderlyingContainer->end(),
			back_inserter(notRetiringElements), ptrOnRetiringCommand);

		commandsHistory = stack<Command*, vector<Command*>>(notRetiringElements);

		delete ptrOnRetiringCommand;
	}

	Command* getLastCommand() {
		return commandsHistory.top();
	}

	Command* getCommandByIndex(int index) {
		return commandsHistory._Get_container()[index];
	}

	void printFileData() {
		cout << "\nІм'я файлу: " << getName() << endl;
		cout << "Шлях до файлу: " << getPath() << endl;
	}

	bool isCommandsHistoryEmpty() {
		return commandsHistory.size() == 0;
	}

	int sizeOfCommandsHistory() {
		return commandsHistory.size();
	}

	void printCommandsHistory();

	Clipboard* getClipboard() {
		return clipboard;
	}
};

class SessionsHistory {
private:
	stack<Session*, vector<Session*>> sessions;

public:
	~SessionsHistory() {
		while (!sessions.empty()) {
			delete sessions.top();
			sessions.pop();
		}
	}

	Session* addSessionToEnd(FileMetadata* fileMetadata) {
		auto session = new Session(fileMetadata);
		sessions.push(session);
		return session;
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

	string deleteLastSession() {
		FileMetadata fileMetadata;
		auto lastSession = sessions.top();
		fileMetadata.setFilename(lastSession->getName());
		fileMetadata.setFilepath(lastSession->getPath());

		delete lastSession;
		sessions.pop();

		return fileMetadata.getFullPath();
	}

	string deleteFirstSession() {
		return deleteSessionByIndex(0);
	}

	string deleteSessionByIndex(int index) {
		FileMetadata fileMetadata;

		auto ptrOnUnderlyingContainer = &sessions._Get_container();
		auto ptrOnRetiringSession = (*ptrOnUnderlyingContainer)[index];

		fileMetadata.setFilename(ptrOnRetiringSession->getName());
		fileMetadata.setFilepath(ptrOnRetiringSession->getPath());

		vector<Session*> notRetiringElements;
		remove_copy(ptrOnUnderlyingContainer->begin(), ptrOnUnderlyingContainer->end(),
			back_inserter(notRetiringElements), ptrOnRetiringSession);

		sessions = stack<Session*, vector<Session*>>(notRetiringElements);

		delete ptrOnRetiringSession;

		return fileMetadata.getFullPath();
	}

	bool isEmpty() {
		return sessions.size() == 0;
	}

	int size() {
		return sessions.size();
	}

	void printSessionsHistory() {
		for (int i = 0; i < sessions.size(); i++)
		{
			cout << "\nСеанс #" << i + 1 << ":\n";
			sessions._Get_container()[i]->printFileData();
		}
		cout << endl;
	}
};

class CopyCommand : public Command {
public:
	CopyCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class DeleteCommand : public Command {
public:
	DeleteCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class CutCommand : public Command {
public:
	CutCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class PasteCommand : public Command {
public:
	PasteCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class UndoCommand : public Command {
public:
	~UndoCommand() {
		if (command)
			delete (command);
	}

	void execute() override;

	void undo() override;

	Command* copy() override;

	Command* getCommand();
};

enum class TypesOfFileMetadata {
	Filename,
	Filepath
};

class FilesManager {
private:
	friend class Editor;

	static const string METADATA_DIRECTORY,
		SESSIONS_DIRECTORY,
		CLIPBOARD_DIRECTORY,
		AVAILABLE_SESSIONS_FILE,
		AVAILABLE_SESSIONS_FULLPATH;

	static bool dataInput(FileMetadata* fileMetadata, TypesOfFileMetadata typeOfData) {
		bool isDataSet = false;
		string data,
			prompt = (typeOfData == TypesOfFileMetadata::Filename) ?
			"\nВведіть ім'я файлу (з розширенням: .txt, .doc або .docx; припинення вводу - \"-1\"): " :
			"\nВведіть шлях до файлу (припинення вводу - \"-1\"): ";

		do {
			cout << prompt;

			getline(cin, data);

			if (data == "-1")
				return false;

			isDataSet = (typeOfData == TypesOfFileMetadata::Filename) ?
				fileMetadata->setFilename(data) :
				fileMetadata->setFilepath(data);

			if (!isDataSet)
				Notification::errorNotification("дані повинні вводитися в правильному форматі та не містити символи '\\', '/', ':', '*', '?', '\"', '<', '>', '|'!");

		} while (!isDataSet);

		return true;
	}

	static void deleteMetadataForDeletedSessions(SessionsHistory* sessionsHistory, string directory) {
		if (!filesystem::exists(SESSIONS_DIRECTORY))
			return;

		stack<string> filesFromMetadataDirectory;
		for (const auto& entry : filesystem::directory_iterator(directory))
			if (entry.path().extension() == ".txt" || entry.path().extension() == ".doc" || entry.path().extension() == ".docx")
				filesFromMetadataDirectory.push(entry.path().string());

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
		deleteMetadataForDeletedSessions(sessionsHistory, SESSIONS_DIRECTORY);

		string filename, typeOfCommand, delimiter = "---\n";
		Session* session;
		Command* command;

		if (!filesystem::exists(SESSIONS_DIRECTORY))
			filesystem::create_directories(SESSIONS_DIRECTORY);

		ofstream ofs_aval_session(AVAILABLE_SESSIONS_FULLPATH);

		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			session = sessionsHistory->getSessionByIndex(i);

			filename = session->getName();
			ofs_aval_session << filename << endl;

			ofstream ofs_session(SESSIONS_DIRECTORY + "\\" + filename);

			ofs_session << session->getName() << endl;
			ofs_session << session->getPath() << endl;
			ofs_session << session->sizeOfCommandsHistory() << endl;

			for (int j = 0; j < session->sizeOfCommandsHistory(); j++)
			{
				command = session->getCommandByIndex(j);

				if (string(typeid(*command).name()) == "class UndoCommand")
					typeOfCommand = "UndoCommand\n";
				else if (string(typeid(*command).name()) == "class CopyCommand")
					typeOfCommand = "CopyCommand\n";
				else if (string(typeid(*command).name()) == "class PasteCommand")
					typeOfCommand = "PasteCommand\n";
				else if (string(typeid(*command).name()) == "class CutCommand")
					typeOfCommand = "CutCommand\n";
				else
					typeOfCommand = "DeleteCommand\n";

				ofs_session << typeOfCommand;

				if (string(typeid(*command).name()) == "class CopyCommand" || string(typeid(*command).name()) == "class UndoCommand")
					continue;

				ofs_session << delimiter;

				ofs_session << command->textToProcess;

				if (command->textToProcess == "")
					ofs_session << endl;

				ofs_session << delimiter;

				ofs_session << command->lastStateOfText;

				if (command->lastStateOfText == "")
					ofs_session << endl;

				ofs_session << delimiter;

				ofs_session << command->startPosition << endl;
				
				ofs_session << command->endPosition << endl;
			}

			ofs_session.close();
		}

		ofs_aval_session.close();
	}

	static void readSessionsMetadata(SessionsHistory* sessionsHistory, Editor* editor) {

		ifstream ifs_available_sessions(AVAILABLE_SESSIONS_FULLPATH);

		if (!ifs_available_sessions) return;

		string line, typeOfCommand, text, delimiter = "---";
		int countOfCommands;
		Session* session;
		Command* command, * previousCommand;

		stack<string> available_sessions;

		while (getline(ifs_available_sessions, line))
			available_sessions.push(line);

		ifs_available_sessions.close();

		for (int i = 0; i < available_sessions.size(); i++)
		{
			ifstream ifs_session(SESSIONS_DIRECTORY + "\\" + available_sessions._Get_container()[i]);

			FileMetadata* fileMetadata = new FileMetadata();

			getline(ifs_session, line);
			fileMetadata->setFilename(line);
			getline(ifs_session, line);
			fileMetadata->setFilepath(line);

			session = new Session(fileMetadata);

			getline(ifs_session, line);
			countOfCommands = stoi(line);

			for (int j = 0; j < countOfCommands; j++)
			{
				getline(ifs_session, line);
				if (line == "UndoCommand")
				{
					command = new UndoCommand();
					previousCommand = session->getCommandByIndex(j - 1);
					command->command = previousCommand;
					session->addCommandAsLast(command);
					continue;
				}
				else if (line == "CopyCommand")
				{
					command = new CopyCommand(editor);
					session->addCommandAsLast(command);
					continue;
				}
				else if (line == "CutCommand")
					command = new CutCommand(editor);
				else if (line == "PasteCommand")
					command = new PasteCommand(editor);
				else
					command = new DeleteCommand(editor);

				getline(ifs_session, line);
				while (getline(ifs_session, line)) {
					if (line == delimiter)
						break;
					text += line + "\n";
				}
				command->textToProcess = text;

				text = "";
				while (getline(ifs_session, line)) {
					if (line == delimiter)
						break;
					text += line + "\n";
				}
				command->lastStateOfText = text;

				getline(ifs_session, line);
				command->startPosition = stoi(line);

				getline(ifs_session, line);
				command->endPosition = stoi(line);

				session->addCommandAsLast(command);
			}

			sessionsHistory->addSessionToEnd(session);

			ifs_session.close();
		}
	}

	static void writeClipboardMetadata(SessionsHistory* sessionsHistory) {
		deleteMetadataForDeletedSessions(sessionsHistory, CLIPBOARD_DIRECTORY);

		string delimiter = "---\n";

		if (!filesystem::exists(CLIPBOARD_DIRECTORY))
			filesystem::create_directories(CLIPBOARD_DIRECTORY);

		Clipboard* clipboard;
		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			ofstream ofs(CLIPBOARD_DIRECTORY + "\\" + sessionsHistory->getSessionByIndex(i)->getName());

			clipboard = sessionsHistory->getSessionByIndex(i)->getClipboard();
			for (int i = 0; i < clipboard->size(); ++i)
			{
				ofs << clipboard->getDataByIndex(i) << endl;
				ofs << delimiter;
			}

			ofs.close();
		}
	}

	static void readClipboardMetadata(SessionsHistory* sessionsHistory) {
		Clipboard* clipboard;
		string data, text;
		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			ifstream ifs(CLIPBOARD_DIRECTORY + "\\" + sessionsHistory->getSessionByIndex(i)->getName());

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
	static FileMetadata* createFileMetadata() {
		FileMetadata* fileMetadata = new FileMetadata();
		return changeFileMetadata(fileMetadata);
	}

	static FileMetadata* changeFileMetadata(FileMetadata* fileMetadata) {
		bool wasDataEntered = dataInput(fileMetadata, TypesOfFileMetadata::Filename) &&
			dataInput(fileMetadata, TypesOfFileMetadata::Filepath);

		if (!wasDataEntered)
			delete fileMetadata;
		return wasDataEntered ? fileMetadata : nullptr;
	}

	static FileMetadata* changeFileMetadata() {
		FileMetadata* fileMetadata = new FileMetadata();

		bool wasDataEntered = dataInput(fileMetadata, TypesOfFileMetadata::Filename) &&
			dataInput(fileMetadata, TypesOfFileMetadata::Filepath);

		if (!wasDataEntered)
			delete fileMetadata;
		return wasDataEntered ? fileMetadata : nullptr;
	}

	static string readFullDataFromFile(string fullFilepath) {
		string text, line;

		ifstream file(fullFilepath);

		while (getline(file, line))
			text += line + "\n";

		file.close();

		return text;
	}

	static bool overwriteDataInExistingFile(string fullFilepath, string newData) {
		ofstream file(fullFilepath);

		if (!file.is_open())
			return false;

		file << newData;
		file.close();

		return true;
	}

	static FileMetadata* createEmptyFile() {
		auto emptyFileMetadata = createFileMetadata();

		if (!emptyFileMetadata)
			return nullptr;

		ofstream file(emptyFileMetadata->getFullPath());
		file.close();

		return emptyFileMetadata;
	}
};

const string FilesManager::METADATA_DIRECTORY = "Metadata",
FilesManager::SESSIONS_DIRECTORY = METADATA_DIRECTORY + "\\" + "Sessions",
FilesManager::CLIPBOARD_DIRECTORY = METADATA_DIRECTORY + "\\" + "Clipboard",
FilesManager::AVAILABLE_SESSIONS_FILE = "Available_Sessions.txt",
FilesManager::AVAILABLE_SESSIONS_FULLPATH = METADATA_DIRECTORY + "\\" + AVAILABLE_SESSIONS_FILE;


class Editor {
private:
	friend class Program;

	static SessionsHistory* sessionsHistory;
	static Session* currentSession;
	static string* currentText;

	void tryToLoadSessions() {
		FilesManager::readSessionsMetadata(sessionsHistory, this);
		FilesManager::readClipboardMetadata(sessionsHistory);
	}

	void tryToUnloadSessions() {
		FilesManager::writeClipboardMetadata(sessionsHistory);
		FilesManager::writeSessionsMetadata(sessionsHistory);
	}
public:
	Editor() {
		this->sessionsHistory = new SessionsHistory();
	}

	~Editor() {
		delete sessionsHistory;
	}

	void copy(string textToProcess, int startPosition, int endPosition) {
		this->currentSession->getClipboard()->addData(textToProcess.substr(startPosition, endPosition - startPosition + 1));
	}

	void paste(string* textToProcess, string textToPaste, int startPosition, int endPosition) {
		if (startPosition == endPosition)
			(*textToProcess).insert(startPosition, textToPaste);
		else
			(*textToProcess).replace(startPosition, endPosition - startPosition + 1, textToPaste);
	}

	void cut(string* textToProcess, int startPosition, int endPosition) {
		copy(*textToProcess, startPosition, endPosition);
		remove(textToProcess, startPosition, endPosition);
	}

	void remove(string* textToProcess, int startPosition, int endPosition) {
		(*textToProcess).erase(startPosition, endPosition - startPosition + 1);
	}

	void setCurrentSession(Session* currentSession) {
		this->currentSession = currentSession;
	}

	static Session* getCurrentSession() {
		return currentSession;
	}

	static SessionsHistory* getSessionsHistory() {
		return sessionsHistory;
	}

	static void setCurrentText(string* text) {
		currentText = text;
	}

	static string* getCurrentText() {
		return currentText;
	}
};

SessionsHistory* Editor::sessionsHistory;
Session* Editor::currentSession;
string* Editor::currentText;

CopyCommand::CopyCommand(Editor* editor) {
	this->editor = editor;
}

void CopyCommand::execute() {
	editor->copy(textToProcess, startPosition, endPosition);
}

void CopyCommand::undo() {
	editor->getCurrentSession()->getClipboard()->deleteLastData();
}

Command* CopyCommand::copy() {
	return new CopyCommand(*this);
}

DeleteCommand::DeleteCommand(Editor* editor) {
	this->editor = editor;
}

void DeleteCommand::execute() {
	lastStateOfText = textToProcess;
	editor->remove(&textToProcess, startPosition, endPosition);
	*ptrOnEditorsText = textToProcess;
}

void DeleteCommand::undo() {
	textToProcess = lastStateOfText;
	*ptrOnEditorsText = lastStateOfText;
}

Command* DeleteCommand::copy() {
	return new DeleteCommand(*this);
}

CutCommand::CutCommand(Editor* editor) {
	this->editor = editor;
}

void CutCommand::execute() {
	lastStateOfText = textToProcess;
	editor->cut(&textToProcess, startPosition, endPosition);
	*ptrOnEditorsText = textToProcess;
}

void CutCommand::undo() {
	textToProcess = lastStateOfText;
	*ptrOnEditorsText = lastStateOfText;
	editor->getCurrentSession()->getClipboard()->deleteLastData();
}

Command* CutCommand::copy() {
	return new CutCommand(*this);
}

PasteCommand::PasteCommand(Editor* editor) {
	this->editor = editor;
}

void PasteCommand::execute() {
	lastStateOfText = textToProcess;
	editor->paste(&textToProcess, textToPaste, startPosition, endPosition);
	*ptrOnEditorsText = textToProcess;
}

void PasteCommand::undo() {
	textToProcess = lastStateOfText;
	*ptrOnEditorsText = lastStateOfText;
}

Command* PasteCommand::copy() {
	return new PasteCommand(*this);
}

void UndoCommand::execute() {
	command->undo();
}

void UndoCommand::undo() {
	if (string(typeid(*command).name()) != "class UndoCommand")
		command->execute();
	else
	{
		UndoCommand* undoCommand = dynamic_cast<UndoCommand*>(command);
		undoCommand->getCommand()->execute();
	}
}

Command* UndoCommand::getCommand() {
	return command;
}

Command* UndoCommand::copy() {
	return new UndoCommand(*this);
}

void Session::printCommandsHistory() {
	if (isCommandsHistoryEmpty()) {
		Notification::errorNotification("за ций сеанс ще не було виконано жодної команди!");
		return;
	}
	for (int i = 0; i < commandsHistory.size(); i++)
	{
		cout << "\n" << i + 1 << ") ";
		if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class UndoCommand")
			cout << "UndoCommand\n";
		else if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class CopyCommand")
			cout << "CopyCommand\n";
		else if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class PasteCommand")
			cout << "PasteCommand\n";
		else if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class CutCommand")
			cout << "CutCommand\n";
		else
			cout << "DeleteCommand\n";
	}
	cout << endl;
}

class CommandsManager {
private:
	map<TypesOfCommands, Command*> manager;
public:
	CommandsManager(Editor* editor) {
		manager[TypesOfCommands::Copy] = new CopyCommand(editor);
		manager[TypesOfCommands::Paste] = new PasteCommand(editor);
		manager[TypesOfCommands::Cut] = new CutCommand(editor);
		manager[TypesOfCommands::Delete] = new DeleteCommand(editor);
		manager[TypesOfCommands::Undo] = new UndoCommand();
	}

	~CommandsManager() {
		for (auto& pair : manager) {
			delete (pair.second);
		}
		manager.clear();
	}

	void invokeCommand(TypesOfCommands typeOfCommand, string* textToProcess = nullptr, int startPosition = 0, int endPosition = 0, string textToPaste = "") {
		Command* command = nullptr;
		if (typeOfCommand == TypesOfCommands::Undo)
			command = Editor::getCurrentSession()->getLastCommand();

		manager[typeOfCommand]->setParameters(typeOfCommand, command, textToProcess, startPosition, endPosition, textToPaste);

		if (command != nullptr && string(typeid(*command).name()) == "class UndoCommand" && !Editor::getCurrentSession()->isCommandsHistoryEmpty()) {
			manager[typeOfCommand]->undo();
			Editor::getCurrentSession()->deleteLastCommand();
			return;
		}

		manager[typeOfCommand]->execute();
		Editor::getCurrentSession()->addCommandAsLast(manager[typeOfCommand]->copy());
	}
};

class Program {
private:
	Editor* editor;
	CommandsManager* commandsManager;

	bool enteredOptionVerification(string option) {
		if (option == "")
			return false;

		for (char ch : option)
			if (!(ch >= '0' && ch <= '9'))
				return false;

		int convertedValue = stoi(option);

		return convertedValue >= INT_MIN && convertedValue <= INT_MAX;
	}

	int optionOfMenuInput() {
		bool isOptionVerified = true;
		string option;

		do {
			cout << "\nВаш вибір: ";
			getline(cin, option);

			if (option == "exit")
			{
				editor->tryToUnloadSessions();
				exit(1);
			}
			else if (option == "info")
				return -1;

			isOptionVerified = enteredOptionVerification(option);

			if (!isOptionVerified)
				Notification::errorNotification("були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!");

		} while (!isOptionVerified);

		return stoi(option);
	}

	bool checkAvailabilityOfSessions() {
		if (editor->getSessionsHistory()->isEmpty())
			Notification::errorNotification("в даний момент жодного сеансу немає!");
		return !editor->getSessionsHistory()->isEmpty();
	}

	void printReferenceInfo() {
		cout << "\nВашої уваги пропонується програму курсової роботи з об'єктно-орієнтованого програмування, побудована за допомогою патерну проектування \"Команда\".\n";
		cout << "За допомогою програми можна створювати, редагувати та видаляти текстові файли, використовуючи команди Копіювання, Вставка, Вирізання та Відміна операції.\n";
		cout << "Задля миттєвого виходу з програми введіть exit, задля отримання довідкової інформації - info.\n";
		cout << "Розробник: Бредун Денис Сергійович з групи ПЗ-21-1/9.\n\n";
	}

	void printMainMenu(int& choice) {
		cout << "Головне меню:\n";
		cout << "0. Закрити програму (команда exit)\n";
		cout << "1. Перейти до меню керування сеансами\n";
		cout << "2. Довідка (команда info)\n";
		choice = optionOfMenuInput();
	}

	void printManagingSessionsMenu(int& choice) {
		cout << "\nМеню керування сеансами:\n";
		cout << "0. Назад\n";
		cout << "1. Створити новий сеанс\n";
		cout << "2. Відкрити існуючий сеанс\n";
		cout << "3. Видалити певний сеанс\n";
		cout << "4. Вивести інформацію про сеанси\n";
		choice = optionOfMenuInput();
	}

	void printGettingSessionsMenu(int& choice) {
		cout << "\nМеню отримання сеансів:\n";
		cout << "0. Назад\n";
		cout << "1. Отримати останній сеанс\n";
		cout << "2. Отримати найперший сеанс\n";
		cout << "3. Отримати сеанс за позицією\n";
		choice = optionOfMenuInput();
	}

	void printDeletingSessionsMenu(int& choice) {
		cout << "\nМеню видалення сеансів:\n";
		cout << "0. Назад\n";
		cout << "1. Видалити останній сеанс\n";
		cout << "2. Видалити найперший сеанс\n";
		cout << "3. Видалити сеанс за позицією\n";
		choice = optionOfMenuInput();
	}

	void printWorkingWithSessionMenu(int& choice) {
		cout << "\nМеню роботи із сеансом:\n";
		cout << "0. Назад\n";
		cout << "1. Переглянути зміст\n";
		cout << "2. Виконати дію над змістом\n";
		cout << "3. Переглянути системні параметри\n";
		cout << "4. Редагувати системні параметри\n";
		cout << "5. Переглянути історію команд\n";
		cout << "6. Переглянути буфер обміну\n";
		cout << "7. Редагувати буфер обміну\n";
		choice = optionOfMenuInput();
	}

	void changeSessionFileMetadata() {
		cout << "Системні параметри файлу " << editor->getCurrentSession()->getName() << " на даний момент:\n";
		editor->getCurrentSession()->printFileData();

		cout << "Введіть нові системні дані:\n";

		FileMetadata* newFileMetadata = FilesManager::changeFileMetadata();

		if (!newFileMetadata) {
			Notification::errorNotification("зміна системних даних була припинена!");
			return;
		}

		if (newFileMetadata->getFilename() != editor->getCurrentSession()->getName()) {
			rename(editor->getCurrentSession()->getFullPath().c_str(), (editor->getCurrentSession()->getPath() + "\\" + newFileMetadata->getFilename()).c_str());
		}
		if (newFileMetadata->getFilepath() != editor->getCurrentSession()->getPath()) {
			rename((editor->getCurrentSession()->getPath() + "\\" + newFileMetadata->getFilename()).c_str(), newFileMetadata->getFullPath().c_str());
		}

		editor->getCurrentSession()->changeName(newFileMetadata->getFilename());
		editor->getCurrentSession()->changePath(newFileMetadata->getFilepath());

		Notification::successNotification("зміна системних даних була успішно виконана!");
	}

	void printWorkingWithClipboard(int& choice) {
		cout << "\nМеню роботи із буфером обміну:\n";
		cout << "0. Назад\n";
		cout << "1. Видалити останні дані\n";
		cout << "2. Видалити найперші дані\n";
		cout << "3. Видалити дані за індексом\n";
		choice = optionOfMenuInput();
	}

	bool deleteDataInClipboardByIndex(int choice = -1) {
		if (choice == -1)
		{
			int choice = optionOfMenuInput();
			if (choice <= 0 || choice > editor->getSessionsHistory()->size())
				return false;
		}

		int indexInCommandsHistory = 0;

		int counter = 0;
		for (int i = 0; i < editor->getCurrentSession()->sizeOfCommandsHistory(); i++)
			if (string(typeid(*editor->getCurrentSession()->getCommandByIndex(i)).name()) == "class CopyCommand")
				if (counter == choice)
				{
					indexInCommandsHistory = i;
					break;
				}
				else
					counter++;

		editor->getCurrentSession()->deleteCommandByIndex(indexInCommandsHistory);

		editor->getCurrentSession()->getClipboard()->deleteDataByIndex(choice);

		return true;
	}

	void executeWorkingWithClipboard() {
		int choice;

		do
		{
			editor->getCurrentSession()->getClipboard()->printClipboard();
			printWorkingWithClipboard(choice);
			switch (choice)
			{
			case -1:
				printReferenceInfo();
				break;
			case 0:
				cout << "\nПовернення до Меню роботи із сеансом.\n\n";
				break;
			case 1:
				if (!editor->getCurrentSession()->getClipboard()->isEmpty())
				{
					deleteDataInClipboardByIndex(editor->getCurrentSession()->getClipboard()->size() - 1);
					Notification::successNotification("останні дані були успішно видалені!");
				}
				else
					Notification::errorNotification("в буфері обміну немає даних!");
				break;
			case 2:
				if (!editor->getCurrentSession()->getClipboard()->isEmpty())
				{
					deleteDataInClipboardByIndex(0);
					Notification::successNotification("останні дані були успішно видалені!");
				}
				else
					Notification::errorNotification("в буфері обміну немає даних!");
				break;
			case 3:
				if (!editor->getCurrentSession()->getClipboard()->isEmpty())
					if (!deleteDataInClipboardByIndex())
						Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
					else
						Notification::successNotification("сеанс був успішно видалений!");
				else
					Notification::errorNotification("в буфері обміну немає даних!");
				break;
			default:
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				break;
			}
		} while (choice != 0);
	}

	void makeActionsOnContentMenu(int& choice) {
		cout << "\nМеню дій над змістом файлу " << editor->getCurrentSession()->getName() << ":\n";
		cout << "0. Назад\n";
		cout << "1. Написати новий текст\n";
		cout << "2. Видалити частину змісту\n";
		cout << "3. Копіювати частину змісту\n";
		cout << "4. Вирізати частину змісту\n";
		cout << "5. Вставити дані з буферу обміну\n";
		cout << "6. Скасувати останню дію\n";
		choice = optionOfMenuInput();
	}

	void addNewTextToContentMenu(int& choice) {
		cout << "\nКуди бажаєте додати текст:\n";
		cout << "0. Повернутись до минулого меню\n";
		cout << "1. В кінець\n";
		cout << "2. На початок\n";
		cout << "3. За певним індексом\n";
		cout << "4. Замінити ним певну частину змісту\n";
		choice = optionOfMenuInput();
	}

	void writeNewTextForContent() {
		string line, text;

		cout << "Введіть новий текст (зупинити ввід - -1):\n";
		while (getline(cin, line)) {
			if (line == "-1")
				break;
			text += line + "\n";
		}

		int choice, startIndex, endIndex;

		addNewTextToContentMenu(choice);
		switch (choice)
		{
		case -1:
			printReferenceInfo();
			break;
		case 0:
			cout << "\nПовернення до Меню дій над змістом файлу.\n\n";
			break;
		case 1:
			if (editor->getCurrentText()->size() == 0) {
				startIndex = 0;
				endIndex = 0;
			}
			else {
				startIndex = editor->getCurrentText()->size()-1;
				endIndex = editor->getCurrentText()->size()-1;
			}
			commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), startIndex, endIndex, text);
			Notification::successNotification("дані були успішно додані в файл!");
			break;
		case 2:
			commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), 0, 0, text);
			Notification::successNotification("дані були успішно додані в файл!");
			break;
		case 3:
			endIndex = optionOfMenuInput();
			if (editor->getCurrentText()->size() == 0) {
				endIndex = 0;
				commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), endIndex, endIndex, text);
				Notification::successNotification("дані були успішно додані в файл!");
			}
			else {
				if (endIndex < 0 || endIndex >= editor->getCurrentText()->size())
					Notification::errorNotification("індекс виходить за межі тексту!");
				else
				{
					commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), endIndex, endIndex, text);
					Notification::successNotification("дані були успішно додані в файл!");
				}
			}
			break;
		case 4:
			cout << "Початковий індекс: ";
			startIndex = optionOfMenuInput();
			cout << "Кінцевий індекс: ";
			endIndex = optionOfMenuInput();

			if (endIndex < 0 || endIndex >= editor->getCurrentText()->size() || startIndex < 0 || startIndex >= editor->getCurrentText()->size())
				Notification::errorNotification("один з індексів виходить за межі тексту!");
			else
			{
				commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), startIndex, endIndex, text);
				Notification::successNotification("дані були успішно додані в файл!");
			}
			break;
		default:
			Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
			break;
		}

		FilesManager::overwriteDataInExistingFile(editor->getCurrentSession()->getFullPath(), *(editor->getCurrentText()));
	}

	void deletePartOfContentMenu(int& choice) {
		cout << "\nЯкий обсяг змісту ви бажаєте видалити:\n";
		cout << "0. Повернутись до минулого меню\n";
		cout << "1. Весь зміст\n";
		cout << "2. З певного по певний індекс\n";
		choice = optionOfMenuInput();
	}

	void deletePartOfContent() {
		int choice, startIndex, endIndex;

		cout << "\nЗміст файлу " << editor->getCurrentSession()->getName() << " в даний момент:\n" << *(editor->getCurrentText()) << endl;
		deletePartOfContentMenu(choice);
		switch (choice)
		{
		case -1:
			printReferenceInfo();
			break;
		case 0:
			cout << "\nПовернення до Меню дій над змістом файлу.\n\n";
			break;
		case 1:
			commandsManager->invokeCommand(TypesOfCommands::Delete, editor->getCurrentText(), 0, editor->getCurrentText()->size() - 1);
			Notification::successNotification("дані були успішно видалені з файлу!");
			break;
		case 2:
			cout << "Початковий індекс: ";
			startIndex = optionOfMenuInput();
			cout << "Кінцевий індекс: ";
			endIndex = optionOfMenuInput();

			if (endIndex < 0 || endIndex >= editor->getCurrentText()->size() || startIndex < 0 || startIndex >= editor->getCurrentText()->size())
				Notification::errorNotification("один з індексів виходить за межі тексту!");
			else
			{
				commandsManager->invokeCommand(TypesOfCommands::Delete, editor->getCurrentText(), startIndex, endIndex);
				Notification::successNotification("дані були успішно видалені з файлу!");
			}
			break;
		default:
			Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
			break;
		}

		FilesManager::overwriteDataInExistingFile(editor->getCurrentSession()->getFullPath(), *(editor->getCurrentText()));
	}

	void copyPartOfContentMenu(int& choice) {
		cout << "\nЯкий обсяг змісту ви бажаєте скопіювати:\n";
		cout << "0. Повернутись до минулого меню\n";
		cout << "1. Весь зміст\n";
		cout << "2. З певного по певний індекс\n";
		choice = optionOfMenuInput();
	}

	void copyPartOfContent() {
		int choice, startIndex, endIndex;

		cout << "\nЗміст файлу " << editor->getCurrentSession()->getName() << " в даний момент:\n" << *(editor->getCurrentText()) << endl;
		copyPartOfContentMenu(choice);
		switch (choice)
		{
		case -1:
			printReferenceInfo();
			break;
		case 0:
			cout << "\nПовернення до Меню дій над змістом файлу.\n\n";
			break;
		case 1:
			commandsManager->invokeCommand(TypesOfCommands::Copy, editor->getCurrentText(), 0, editor->getCurrentText()->size() - 1);
			Notification::successNotification("дані були успішно скопійовані!");
			break;
		case 2:
			cout << "Початковий індекс: ";
			startIndex = optionOfMenuInput();
			cout << "Кінцевий індекс: ";
			endIndex = optionOfMenuInput();

			if (endIndex < 0 || endIndex >= editor->getCurrentText()->size() || startIndex < 0 || startIndex >= editor->getCurrentText()->size())
				Notification::errorNotification("один з індексів виходить за межі тексту!");
			else
			{
				commandsManager->invokeCommand(TypesOfCommands::Copy, editor->getCurrentText(), startIndex, endIndex);
				Notification::successNotification("дані були успішно скопійовані!");
			}
			break;
		default:
			Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
			break;
		}

		FilesManager::overwriteDataInExistingFile(editor->getCurrentSession()->getFullPath(), *(editor->getCurrentText()));
	}

	void cutPartOfContentMenu(int& choice) {
		cout << "\nЯкий обсяг змісту ви бажаєте вирізати:\n";
		cout << "0. Повернутись до минулого меню\n";
		cout << "1. Весь зміст\n";
		cout << "2. З певного по певний індекс\n";
		choice = optionOfMenuInput();
	}

	void cutPartOfContent() {
		int choice, startIndex, endIndex;

		cout << "\nЗміст файлу " << editor->getCurrentSession()->getName() << " в даний момент:\n" << *(editor->getCurrentText()) << endl;
		cutPartOfContentMenu(choice);
		switch (choice)
		{
		case -1:
			printReferenceInfo();
			break;
		case 0:
			cout << "\nПовернення до Меню дій над змістом файлу.\n\n";
			break;
		case 1:
			commandsManager->invokeCommand(TypesOfCommands::Cut, editor->getCurrentText(), 0, editor->getCurrentText()->size() - 1);
			Notification::successNotification("дані були успішно вирізані!");
			break;
		case 2:
			cout << "Початковий індекс: ";
			startIndex = optionOfMenuInput();
			cout << "Кінцевий індекс: ";
			endIndex = optionOfMenuInput();

			if (endIndex < 0 || endIndex >= editor->getCurrentText()->size() || startIndex < 0 || startIndex >= editor->getCurrentText()->size())
				Notification::errorNotification("один з індексів виходить за межі тексту!");
			else
			{
				commandsManager->invokeCommand(TypesOfCommands::Cut, editor->getCurrentText(), startIndex, endIndex);
				Notification::successNotification("дані були успішно вирізані!");
			}
			break;
		default:
			Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
			break;
		}

		FilesManager::overwriteDataInExistingFile(editor->getCurrentSession()->getFullPath(), *(editor->getCurrentText()));
	}

	void pasteTextIntoContentMenu(int& choice) {
		cout << "\nКуди бажаєте вставити текст:\n";
		cout << "0. Повернутись до минулого меню\n";
		cout << "1. В кінець\n";
		cout << "2. На початок\n";
		cout << "3. За певним індексом\n";
		cout << "4. Замінити ним певну частину змісту\n";
		choice = optionOfMenuInput();
	}

	void getDataFromClipboardMenu(int& choice) { //вже було схоже меню
		cout << "\nЯкі дані бажаєте отримати з буферу обміну:\n";
		cout << "0. Повернутись до минулого меню\n";
		cout << "1. Останні\n";
		cout << "2. Найперші\n";
		cout << "3. За індексом\n";
		choice = optionOfMenuInput();
	}

	void pasteDataFromClipboard() { //дуже схоже меню з меню для додавання тексту
		int choice, startIndex, endIndex;
		string textToPaste;

		editor->getCurrentSession()->getClipboard()->printClipboard();

		getDataFromClipboardMenu(choice);

		switch (choice)
		{
		case -1:
			printReferenceInfo();
			break;
		case 0:
			cout << "\nПовернення до Меню дій над змістом файлу.\n\n";
			break;
		case 1:
			textToPaste = editor->getCurrentSession()->getClipboard()->getLastData();
			break;
		case 2:
			textToPaste = editor->getCurrentSession()->getClipboard()->getDataByIndex(0);
			break;
		case 3:
		{
			endIndex = optionOfMenuInput(); //можно сделать так, чтоб вместо "Ваш выбор" вывводилось передаваемое туда сообщение
			if (endIndex <= 0 || endIndex > editor->getCurrentSession()->getClipboard()->size())
			{
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				return;
			}
			else
				textToPaste = editor->getCurrentSession()->getClipboard()->getDataByIndex(endIndex - 1);
			break;
		}
		default:
			Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
			return;
		}

		cout << "\nЗміст файлу " << editor->getCurrentSession()->getName() << " в даний момент:\n" << *(editor->getCurrentText()) << endl;
		pasteTextIntoContentMenu(choice);
		switch (choice)
		{
		case -1:
			printReferenceInfo();
			break;
		case 0:
			cout << "\nПовернення до Меню дій над змістом файлу.\n\n";
			break;
		case 1:
			commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), editor->getCurrentText()->size() - 1, editor->getCurrentText()->size() - 1, textToPaste);
			Notification::successNotification("дані були успішно додані в файл!");
			break;
		case 2:
			commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), 0, 0, textToPaste);
			Notification::successNotification("дані були успішно додані в файл!");
			break;
		case 3:
			endIndex = optionOfMenuInput();
			if (editor->getCurrentText()->size() == 0) {
				endIndex = 0;
				commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), endIndex, endIndex, textToPaste);
				Notification::successNotification("дані були успішно додані в файл!");
			}
			else {
				if (endIndex < 0 || endIndex >= editor->getCurrentText()->size())
					Notification::errorNotification("індекс виходить за межі тексту!");
				else
				{
					commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), endIndex, endIndex, textToPaste);
					Notification::successNotification("дані були успішно додані в файл!");
				}
			}
			break;
		case 4:
			cout << "Початковий індекс: ";
			startIndex = optionOfMenuInput();
			cout << "Кінцевий індекс: ";
			endIndex = optionOfMenuInput();

			if (endIndex < 0 || endIndex >= editor->getCurrentText()->size() || startIndex < 0 || startIndex >= editor->getCurrentText()->size())
				Notification::errorNotification("один з індексів виходить за межі тексту!");
			else
			{
				commandsManager->invokeCommand(TypesOfCommands::Paste, editor->getCurrentText(), startIndex, endIndex, textToPaste);
				Notification::successNotification("дані були успішно додані в файл!");
			}
			break;
		default:
			Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
			break;
		}

		FilesManager::overwriteDataInExistingFile(editor->getCurrentSession()->getFullPath(), *(editor->getCurrentText()));
	}

	void cancelLastAction() {
		commandsManager->invokeCommand(TypesOfCommands::Undo);
		FilesManager::overwriteDataInExistingFile(editor->getCurrentSession()->getFullPath(), *(editor->getCurrentText()));
	}

	void executeMakeActionsOnContentMenu() {
		int choice;
		commandsManager = new CommandsManager(editor);
		do
		{
			cout << "\nЗміст файлу " << editor->getCurrentSession()->getName() << " в даний момент:\n" << *(editor->getCurrentText()) << endl;
			makeActionsOnContentMenu(choice);
			switch (choice)
			{
			case -1:
				printReferenceInfo();
				break;
			case 0:
				cout << "\nПовернення до Меню роботи із сеансом.\n\n";
				delete (commandsManager);
				break;
			case 1:
				writeNewTextForContent();
				break;
			case 2:
				if(editor->getCurrentText()->size() > 0)
					deletePartOfContent();
				else
					Notification::errorNotification("немає тексту, який можна було б видалити!");
				break;
			case 3:
				if (editor->getCurrentText()->size() > 0)
					copyPartOfContent();
				else
					Notification::errorNotification("немає тексту, який можна було б скопіювати!");
				break;
			case 4:
				if (editor->getCurrentText()->size() > 0)
					cutPartOfContent();
				else
					Notification::errorNotification("немає тексту, який можна було б вирізати!");
				break;
			case 5:
				if (!editor->getCurrentSession()->getClipboard()->isEmpty())
					pasteDataFromClipboard();
				else
					Notification::errorNotification("в буфері обміну немає даних!");
				break;
			case 6:
				if(editor->getCurrentSession()->sizeOfCommandsHistory() > 0)
					cancelLastAction();
				else
					Notification::errorNotification("немає дій, які можна було б відмінити!");
				break;
			default:
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				break;
			}
		} while (choice != 0);
	}

	void executeWorkingWithSessionMenu() {
		int choice;

		string* textFromFile = new string(FilesManager::readFullDataFromFile(editor->currentSession->getFullPath()));
		editor->setCurrentText(textFromFile);
		
		if (!editor->getCurrentSession()->isCommandsHistoryEmpty()) {
			Command* command;
			for (int i = 0; i < editor->getCurrentSession()->sizeOfCommandsHistory(); i++)
			{
				command = editor->getCurrentSession()->getCommandByIndex(i);
				command->setPtrOnEditorsText(textFromFile);
			}
		}

		do
		{
			printWorkingWithSessionMenu(choice);
			switch (choice)
			{
			case -1:
				printReferenceInfo();
				break;
			case 0:
				cout << "\nПовернення до Меню отримання сеансів.\n\n";
				delete textFromFile;
				break;
			case 1:
				cout << "\nЗміст файлу " << editor->getCurrentSession()->getName() << " в даний момент:\n" << *(editor->getCurrentText()) << endl;
				break;
			case 2:
				executeMakeActionsOnContentMenu();
				break;
			case 3:
				cout << "Системні параметри файлу " << editor->getCurrentSession()->getName() << ":\n";
				editor->getCurrentSession()->printFileData();
				break;
			case 4:
				changeSessionFileMetadata();
				break;
			case 5:
				cout << "Історія команд файлу " << editor->getCurrentSession()->getName() << ":\n";
				editor->getCurrentSession()->printCommandsHistory();
				break;
			case 6:
				cout << "Буфер обміну файлу " << editor->getCurrentSession()->getName() << ":\n";
				editor->getCurrentSession()->getClipboard()->printClipboard();
				break;
			case 7:
				if (!editor->getCurrentSession()->getClipboard()->isEmpty())
					executeWorkingWithClipboard();
				else
					Notification::errorNotification("в буфері обміну немає даних!");
				break;
			default:
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				break;
			}
		} while (choice != 0);
	}

	void createSession() {
		auto fileMetadata = FilesManager::createEmptyFile();
		if (fileMetadata)
		{
			editor->getSessionsHistory()->addSessionToEnd(fileMetadata);
			Notification::successNotification("сеанс був успішно створений!");
		}
		else
			Notification::errorNotification("створення сеансу було припинено!");
	}

	bool deleteSessionByIndex() {
		int choice = optionOfMenuInput();
		if (choice <= 0 || choice > editor->getSessionsHistory()->size())
			return false;
		string pathOfSession = editor->getSessionsHistory()->deleteSessionByIndex(choice - 1);
		remove(pathOfSession.c_str());
		return true;
	}

	bool setCurrentSessionByIndex() {
		int choice = optionOfMenuInput();
		if (choice <= 0 || choice > editor->getSessionsHistory()->size())
			return false;
		editor->setCurrentSession(editor->getSessionsHistory()->getSessionByIndex(choice - 1));
		return true;
	}

	void executeGettingSessionsMenuLogic() {
		int choice;
		do
		{
			editor->getSessionsHistory()->printSessionsHistory();
			printGettingSessionsMenu(choice);
			switch (choice)
			{
			case -1:
				printReferenceInfo();
				break;
			case 0:
				cout << "\nПовернення до Меню керування сеансами.\n\n";
				break;
			case 1:
				editor->setCurrentSession(editor->getSessionsHistory()->getLastSession());
				executeWorkingWithSessionMenu();
				break;
			case 2:
				editor->setCurrentSession(editor->getSessionsHistory()->getFirstSession());
				executeWorkingWithSessionMenu();
				break;
			case 3:
				if (!setCurrentSessionByIndex())
					Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				else
					executeWorkingWithSessionMenu();
				break;
			default:
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				break;
			}
		} while (choice != 0);
	}

	void executeDeletingSessionsMenuLogic() {
		int choice;
		string pathOfSession;
		do
		{
			editor->getSessionsHistory()->printSessionsHistory();
			printDeletingSessionsMenu(choice);
			switch (choice)
			{
			case -1:
				printReferenceInfo();
				break;
			case 0:
				cout << "\nПовернення до Меню керування сеансами.\n\n";
				break;
			case 1:
				if (!checkAvailabilityOfSessions())
					continue;
				pathOfSession = editor->getSessionsHistory()->deleteLastSession();
				remove(pathOfSession.c_str());
				Notification::successNotification("сеанс був успішно видалений!");
				break;
			case 2:
				if (!checkAvailabilityOfSessions())
					continue;
				pathOfSession = editor->getSessionsHistory()->deleteFirstSession();
				remove(pathOfSession.c_str());
				Notification::successNotification("сеанс був успішно видалений!");
				break;
			case 3:
				if (!checkAvailabilityOfSessions())
					continue;
				if (!deleteSessionByIndex())
					Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				else
					Notification::successNotification("сеанс був успішно видалений!");
				break;
			default:
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				break;
			}
		} while (choice != 0);
	}

	void executeManagingSessionsMenuLogic() {
		int choice;
		editor = new Editor();
		editor->tryToLoadSessions();

		do
		{
			printManagingSessionsMenu(choice);
			switch (choice)
			{
			case -1:
				printReferenceInfo();
				break;
			case 0:
				cout << "\nПовернення до Головного меню.\n\n";
				editor->tryToUnloadSessions();
				delete editor;
				break;
			case 1:
				createSession();
				break;
			case 2:
				if (!checkAvailabilityOfSessions())
					continue;
				executeGettingSessionsMenuLogic();
				break;
			case 3:
				if (!checkAvailabilityOfSessions())
					continue;
				executeDeletingSessionsMenuLogic();
				break;
			case 4:
				if (!checkAvailabilityOfSessions())
					continue;
				editor->sessionsHistory->printSessionsHistory();
				break;
			default:
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				break;
			}
		} while (choice != 0);
	}

public:
	void startProgram() {
		int choice;
		do
		{
			printMainMenu(choice);
			switch (choice)
			{
			case -1:
				printReferenceInfo();
				break;
			case 0:
				cout << "\nДякую за користування! До побачення! =)\n";
				break;
			case 1:
				executeManagingSessionsMenuLogic();
				break;
			case 2:
				printReferenceInfo();
				break;
			default:
				Notification::errorNotification("було введено число, яке виходить за межі набору цифр наданих варіантів!");
				break;
			}
		} while (choice != 0);
	}
};

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	Program* program = new Program();
	program->startProgram();
	delete program;
}

//Заметки:

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








//упорядочить методы по группам

//много где используется optionOfMenuInput, хотя там надо просто верификация введённого числа

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

Помилка: було введено число, яке виходить за межі набору цифр наданих варіантів!
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
//if (!checkAvailabilityOfSessions())
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