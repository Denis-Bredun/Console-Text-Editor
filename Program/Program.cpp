#include <iostream>
#include <fstream>
#include <filesystem>
#include <stack>
#include <map>
#include <functional>
#include <windows.h>

class Session;
class SessionsHistory;

class Editor {
private:
	friend class Program;

	static SessionsHistory* sessionsHistory; //історія сеансів
	static Session* currentSession; //сеанс, з яким користувач працює в даний момент
	static std::string* currentText; //текст, який користувач редагує в даний момент

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
	Editor* editor; //редактор, в якому відбувається редагування тексту за допомогою команд
	int startPosition, endPosition; //початкова та кінцева позиції для вставки, заміни, видалення, копіювання, вирізання
	std::string textToProcess, textToPaste; //поля для тексту, який обробляємо і для тексту, який вставляємо 
	Command* previousCommand, *commandToUndoOrRedo; //вказівник на попередню команду (в історії команд щось по типу однонапрямленого списка),
													//далі - вказівник на команду, яку збираємось скасувати або повторити

public:
	virtual void execute() = 0;
	virtual void undo() = 0;
	virtual Command* copy() = 0;

	void setParameters(std::string typeOfCommand, Command* previousCommand, Command* commandToUndoOrRedo, int startPosition, int endPosition, std::string textToPaste) {
		if (typeOfCommand == "Undo" || typeOfCommand == "Redo")
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

		if (typeOfCommand == "Paste")
			this->textToPaste = textToPaste;
	}

	std::string getTextToProcess() { return textToProcess; }
	void setTextToProcess(std::string textToProcess) { this->textToProcess = textToProcess; }
	void setPreviousCommand(Command* previousCommand) { this->previousCommand = previousCommand; }
};

class Session {
private:
	std::stack<Command*> commandsHistory; //історія команд
	std::stack<std::string> clipboard; //буфер обміну
	int currentCommandIndexInHistory; //індекс на команді, на якій знаходиться користувач, бо, можливо, він скасував декілька команд або повторив,
									  //і це потрібно відслідковвувати
	std::string name; //ім'я сеансу

public:
	Session() { currentCommandIndexInHistory = -1; }
	Session(std::string filename) : Session() { name = filename; }
	~Session() {
		while (!commandsHistory.empty()) {
			if (commandsHistory.top())
				delete commandsHistory.top();
			commandsHistory.pop();
		}
	}

	void addCommandAsLast(Command* command) { commandsHistory.push(command); }
	void addDataToClipboard(std::string data) { clipboard.push(data); }
	void deleteLastCommand() {
		delete commandsHistory.top();
		commandsHistory.pop();
	}

	int sizeOfCommandsHistory() { return commandsHistory.size(); }
	int sizeOfClipboard() { return clipboard.size(); }

	bool setName(std::string filename) {
		std::string forbiddenCharacters = "/\\\":?*|<>";

		for (char ch : forbiddenCharacters)
			if (filename.find(ch) != std::string::npos)
				return false;

		name = filename + ".txt";
		return true;
	}
	void setCurIndexInCommHistory(int currentCommandIndexInHistory) {
		this->currentCommandIndexInHistory = currentCommandIndexInHistory;
	}

	std::string getName() { return name; }
	Command* getCommandByIndex(int index) { return commandsHistory._Get_container()[index]; }
	int getCurIndexInCommHistory() { return currentCommandIndexInHistory; }
	std::string getDataFromClipboardByIndex(int index) { return clipboard._Get_container()[index]; }

	void printClipboard() {
		system("cls");
		for (int i = 0; i < clipboard.size(); i++)
			std::cout << "\n" << i + 1 << ") \"" << clipboard._Get_container()[i] << "\"";
		std::cout << std::endl;
	}
};

class SessionsHistory {
private:
	std::stack<Session*> sessions; //історія сеансів

public:
	~SessionsHistory() {
		while (!sessions.empty()) {
			delete sessions.top();
			sessions.pop();
		}
	}

	void addSessionToEnd(Session* session) { sessions.push(session); }
	Session* getSessionByIndex(int index) { return sessions._Get_container()[index]; }
	Session* getSessionByName(std::string name) {
		auto sessionsIter = std::find_if(sessions._Get_container().begin(), sessions._Get_container().end(), [name](Session* session) {
			return session->getName() == name + ".txt" || session->getName() == name;
			});

		if (sessionsIter != sessions._Get_container().end())
			return *sessionsIter;
		else
			return nullptr;
	}
	std::string deleteSessionByIndex(int index) {
		auto ptrOnUnderlyingContainer = &sessions._Get_container();
		auto ptrOnRetiringSession = (*ptrOnUnderlyingContainer)[index];

		std::string filename = ptrOnRetiringSession->getName();

		std::deque<Session*> notRetiringElements;

		remove_copy(ptrOnUnderlyingContainer->begin(), 
					ptrOnUnderlyingContainer->end(),
					back_inserter(notRetiringElements), 
					ptrOnRetiringSession);

		sessions = std::stack<Session*>(notRetiringElements);

		delete ptrOnRetiringSession;

		return filename;
	}
	std::string deleteSessionByName(std::string name) {
		auto sessionToDelete = getSessionByName(name);
		if (!sessionToDelete)
			return "";

		Session* topSession;
		std::string filename = sessionToDelete->getName();
		std::stack<Session*> tempStack;


		while (!sessions.empty()) {
			topSession = sessions.top();
			sessions.pop();

			if (topSession->getName() == name) {
				delete topSession;
				continue;
			}

			tempStack.push(topSession);
		}

		while (!tempStack.empty()) {
			sessions.push(tempStack.top());
			tempStack.pop();
		}

		return filename;
	}

	bool isEmpty() { return sessions.size() == 0; }
	int size() { return sessions.size(); }

	void printSessionsHistory() {
		system("cls");
		for (int i = 0; i < sessions.size(); i++)
			std::cout << "\nСеанс #" << i + 1 << ": " << sessions._Get_container()[i]->getName();
		std::cout << std::endl;
	}

	void sortByName() {
		std::deque<Session*> tempSessions;

		while (!sessions.empty()) {
			tempSessions.push_back(sessions.top());
			sessions.pop();
		}

		std::sort(tempSessions.begin(), tempSessions.end(), [](Session* a, Session* b) {
			return a->getName() < b->getName();
		});

		for (Session* session : tempSessions) 
			sessions.push(session);
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
		if (commandToUndoOrRedo)
			delete (commandToUndoOrRedo);
	}

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class RedoCommand : public Command {
public:
	~RedoCommand() {
		if (commandToUndoOrRedo)
			delete (commandToUndoOrRedo);
	}

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class FilesManager {
private:
	friend class Editor;

	static const std::string METADATA_DIRECTORY, //директорія папки метаданих
		DATA_DIRECTORY; //директорія, де безпосередньо збергаються текстові файли, які ми редагуємо в програмі

	static std::stack<std::string> getFilepathsForMetadata(std::string directory) {
		std::stack<std::string> filesFromMetadataDirectory;

		auto iteratorOnFiles = std::filesystem::directory_iterator(directory);

		for (const auto& entry : iteratorOnFiles)
				filesFromMetadataDirectory.push(entry.path().string());

		return filesFromMetadataDirectory;
	}
	static std::string readDataByDelimiter(std::ifstream* ifs_session, std::string delimiter) {
		std::string line, text;
		int counterOfLines = 0;

		getline(*ifs_session, line);
		while (getline(*ifs_session, line)) {
			if (line == delimiter)
				break;
			counterOfLines++;
			if (counterOfLines > 1)
				line = "\n" + line;
			text += line;
		}

		return text;
	}

	static void deleteMetadataForDeletedSessions(SessionsHistory* sessionsHistory, std::string directory) {
		if (!std::filesystem::exists(directory))
			return;

		std::stack<std::string> filesFromMetadataDirectory = getFilepathsForMetadata(directory);
		std::string sessionFilepath;
		bool isPartOfRealtimeSessions;

		for (int i = 0; i < filesFromMetadataDirectory.size(); i++)
		{
			isPartOfRealtimeSessions = false;
			for (int j = 0; j < sessionsHistory->size(); j++)
			{
				sessionFilepath = directory + sessionsHistory->getSessionByIndex(j)->getName();
				if (filesFromMetadataDirectory._Get_container()[i] == sessionFilepath)
					isPartOfRealtimeSessions = true;
			}

			if (!isPartOfRealtimeSessions)
				remove(filesFromMetadataDirectory._Get_container()[i].c_str());
		}
		
	}

	static void writeSessionsMetadata(SessionsHistory* sessionsHistory) {
		deleteMetadataForDeletedSessions(sessionsHistory, METADATA_DIRECTORY);

		if (!std::filesystem::exists(METADATA_DIRECTORY))
			std::filesystem::create_directories(METADATA_DIRECTORY);

		for (int i = 0; i < sessionsHistory->size(); i++)
			writeSessionMetadata(sessionsHistory, i);
	}
	static void writeSessionMetadata(SessionsHistory* sessionsHistory, int index) {
		Session* session = sessionsHistory->getSessionByIndex(index);

		std::ofstream ofs_session(METADATA_DIRECTORY + session->getName());

		ofs_session << session->sizeOfCommandsHistory() << std::endl;
		ofs_session << session->getCurIndexInCommHistory() << std::endl;

		for (int j = 0; j < session->sizeOfCommandsHistory(); j++)
			writeCommandMetadata(&ofs_session, session, j);

		ofs_session.close();
	}
	static void writeCommandMetadata(std::ofstream* ofs_session, Session* session, int index) {
		std::string typeOfCommand, nameOfCommandClass, delimiter = "---\n";
		Command* command = session->getCommandByIndex(index);

		nameOfCommandClass = std::string(typeid(*command).name());

		if (nameOfCommandClass == "class PasteCommand")
			typeOfCommand = "PasteCommand";
		else if (nameOfCommandClass == "class CutCommand")
			typeOfCommand = "CutCommand";
		else
			typeOfCommand = "DeleteCommand";

		*ofs_session << typeOfCommand << std::endl << delimiter;

		if (command->getTextToProcess() != "")
			*ofs_session << command->getTextToProcess() << std::endl;

		*ofs_session << delimiter;
	}

	static void readSessionsMetadata(SessionsHistory* sessionsHistory, Editor* editor) {
		std::stack<std::string> available_sessions;
		std::string line;

		available_sessions = getFilepathsForMetadata(METADATA_DIRECTORY);

		for (int i = 0; i < available_sessions.size(); i++)
			readSessionMetadata(sessionsHistory, editor, available_sessions._Get_container()[i]);
	}
	static void readSessionMetadata(SessionsHistory* sessionsHistory, Editor* editor, std::string sessionFilename) {
		std::string line;
		Session* session = new Session(sessionFilename);
		int countOfCommands;

		std::ifstream ifs_session(METADATA_DIRECTORY + sessionFilename);

		getline(ifs_session, line);
		countOfCommands = stoi(line);

		getline(ifs_session, line);
		session->setCurIndexInCommHistory(stoi(line));

		for (int j = 0; j < countOfCommands; j++)
			readCommandMetadata(editor, &ifs_session, session);

		sessionsHistory->addSessionToEnd(session);

		ifs_session.close();
	}
	static void readCommandMetadata(Editor* editor, std::ifstream* ifs_session, Session* session) {
		std::string typeOfCommand, text;
		Command* command,* previousCommand;

		getline(*ifs_session, typeOfCommand);
		if (typeOfCommand == "CutCommand")
			command = new CutCommand(editor);
		else if (typeOfCommand == "PasteCommand")
			command = new PasteCommand(editor);
		else
			command = new DeleteCommand(editor);

		if (session->sizeOfCommandsHistory() > 0)
		{
			previousCommand = session->getCommandByIndex(session->sizeOfCommandsHistory() - 1);
			command->setPreviousCommand(previousCommand);
		}

		text = readDataByDelimiter(ifs_session, "---");
		command->setTextToProcess(text);

		session->addCommandAsLast(command);
	}

public:
	static std::string getSessionsDirectory() {
		return DATA_DIRECTORY;
	}

	static std::string readSessionData(std::string fullFilepath) {
		std::string text, line;

		std::ifstream file(fullFilepath);

		int counterOfLines = 0;

		while (getline(file, line))
		{
			counterOfLines++;
			if (counterOfLines > 1)
				line = "\n" + line;
			text += line;
		}

		file.close();

		return text;
	}

	static bool writeSessionData(std::string filename, std::string newData) {
		std::ofstream file(METADATA_DIRECTORY + filename);

		if (!file.is_open())
			return false;

		file << newData;
		if(!newData.empty() && newData[newData.size() - 1] == '\n')
			file << '\n';

		file.close();

		return true;
	}
};

const std::string FilesManager::METADATA_DIRECTORY = "Metadata\\",
FilesManager::DATA_DIRECTORY = "Sessions\\";

void Editor::tryToLoadSessions() { FilesManager::readSessionsMetadata(sessionsHistory, this); }
void Editor::tryToUnloadSessions() { FilesManager::writeSessionsMetadata(sessionsHistory); }

Editor::Editor() { this->sessionsHistory = new SessionsHistory(); }

void Editor::copy(std::string textToProcess, int startPosition, int endPosition) {
	std::string dataToCopy = textToProcess.substr(startPosition, endPosition - startPosition + 1);
	currentSession->addDataToClipboard(dataToCopy);
}
void Editor::paste(std::string* textToProcess, int startPosition, int endPosition, std::string textToPaste) {
	if (startPosition == endPosition)
	{
		if (startPosition == textToProcess->size() - 1)
			*textToProcess += textToPaste;
		else if(startPosition == 0)
			*textToProcess = textToPaste + *textToProcess;
		else
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

Session* Editor::getCurrentSession() { return currentSession; }
std::string* Editor::getCurrentText() { return currentText; }

void Editor::printCurrentText() {
	system("cls");
	std::cout << "\nЗміст файлу " << currentSession->getName() << ":\n";
	*currentText != "" ? 
		std::cout << "\"" << *currentText << "\"\n" : 
		std::cout << "\nФайл пустий!\n";
}

SessionsHistory* Editor::sessionsHistory;
Session* Editor::currentSession;
std::string* Editor::currentText;

CopyCommand::CopyCommand(Editor* editor) { this->editor = editor; }

void CopyCommand::execute() { editor->copy(textToProcess, startPosition, endPosition); }
void CopyCommand::undo() { }
Command* CopyCommand::copy() { return nullptr; }

DeleteCommand::DeleteCommand(Editor* editor) { this->editor = editor; }

void DeleteCommand::execute() { editor->remove(&textToProcess, startPosition, endPosition); }
void DeleteCommand::undo() { *(Editor::getCurrentText()) = previousCommand->getTextToProcess(); }
Command* DeleteCommand::copy() { return new DeleteCommand(*this); }

CutCommand::CutCommand(Editor* editor) { this->editor = editor; }

void CutCommand::execute() { editor->cut(&textToProcess, startPosition, endPosition); }
void CutCommand::undo() { *(Editor::getCurrentText()) = previousCommand->getTextToProcess(); }
Command* CutCommand::copy() { return new CutCommand(*this); }

PasteCommand::PasteCommand(Editor* editor) { this->editor = editor; }

void PasteCommand::execute() { editor->paste(&textToProcess, startPosition, endPosition, textToPaste); }
void PasteCommand::undo() {
	if (previousCommand)
		*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
	else
		*(Editor::getCurrentText()) = "";
}
Command* PasteCommand::copy() { return new PasteCommand(*this); }

void UndoCommand::execute() { commandToUndoOrRedo->undo(); }
void UndoCommand::undo() { }
Command* UndoCommand::copy() { return nullptr; }

void RedoCommand::execute() { *(Editor::getCurrentText()) = commandToUndoOrRedo->getTextToProcess(); }
void RedoCommand::undo() { }
Command* RedoCommand::copy() { return nullptr; }

class CommandsManager {
private:
	std::stack<std::pair<std::string, Command*>> manager; //зберігач усіх команд, дозволяє зручно їми керувати за допомогою поліморфізму

	Command* getCommandFromManagerByKey(std::string typeOfCommand) {
		for (int i = 0; i < manager.size(); i++)
			if (manager._Get_container()[i].first == typeOfCommand)
				return manager._Get_container()[i].second;
	}
	bool isNotUndoOrRedoCommand(std::string typeOfCommand) { return typeOfCommand != "Undo" && typeOfCommand != "Redo"; }
	void setParametersForCommand(std::string typeOfCommand, int startPosition, int endPosition, std::string textToPaste) {
		Command* commandToUndoOrRedo = nullptr, * previousCommand = nullptr;

		if (typeOfCommand == "Undo")
			commandToUndoOrRedo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurIndexInCommHistory());

		if(typeOfCommand == "Redo")
			commandToUndoOrRedo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurIndexInCommHistory() + 1);

		if (Editor::getCurrentSession()->getCurIndexInCommHistory() != -1 && isNotUndoOrRedoCommand(typeOfCommand))
			previousCommand = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->sizeOfCommandsHistory() - 1);

		

		getCommandFromManagerByKey(typeOfCommand)->setParameters(typeOfCommand, previousCommand, commandToUndoOrRedo, startPosition, endPosition, textToPaste);
	}
	int getCountOfForwardCommands() {
		return Editor::getCurrentSession()->sizeOfCommandsHistory() - 1 - Editor::getCurrentSession()->getCurIndexInCommHistory();
	}
	void deleteForwardCommandsIfNecessary(std::string typeOfCommand) {
		if (typeOfCommand != "Undo" && typeOfCommand != "Redo")
			if (isThereAnyCommandForward()) {
				for (int i = 1; i <= getCountOfForwardCommands(); i++)
					Editor::getCurrentSession()->deleteLastCommand();
			}
	}

public:
	CommandsManager(Editor* editor) {
		manager.push(std::pair("Copy", new CopyCommand(editor)));
		manager.push(std::pair("Paste", new PasteCommand(editor)));
		manager.push(std::pair("Cut", new CutCommand(editor)));
		manager.push(std::pair("Delete", new DeleteCommand(editor)));
		manager.push(std::pair("Undo", new UndoCommand()));
		manager.push(std::pair("Redo", new RedoCommand()));
	}
	~CommandsManager() {
		while (!manager.empty())
		{
			delete manager.top().second;
			manager.pop();
		}
	}

	bool isThereAnyCommandForward() {
		return Editor::getCurrentSession()->sizeOfCommandsHistory() != 0 &&
			Editor::getCurrentSession()->getCurIndexInCommHistory() < Editor::getCurrentSession()->sizeOfCommandsHistory() - 1;
	}
	void invokeCommand(std::string typeOfCommand, int startPosition = 0, int endPosition = 0, std::string textToPaste = "") {

		deleteForwardCommandsIfNecessary(typeOfCommand);
		setParametersForCommand(typeOfCommand, startPosition, endPosition, textToPaste);

		getCommandFromManagerByKey(typeOfCommand)->execute();

		if (isNotUndoOrRedoCommand(typeOfCommand) && typeOfCommand != "Copy")
			Editor::getCurrentSession()->addCommandAsLast(getCommandFromManagerByKey(typeOfCommand)->copy());

		if(typeOfCommand == "Undo")
			Editor::getCurrentSession()->setCurIndexInCommHistory(Editor::getCurrentSession()->getCurIndexInCommHistory() - 1);
		else
			if (typeOfCommand != "Copy")
				Editor::getCurrentSession()->setCurIndexInCommHistory(Editor::getCurrentSession()->getCurIndexInCommHistory() + 1);
	}
};

class Program {
private:
	Editor* editor; //редактор
	CommandsManager* commandsManager; //менеджер команд, за допомогою якого й викликаються усі команди

	bool validateEnteredNumber(std::string option, short firstOption, short lastOption) {
		if (option.empty())
			return false;

		for (char num : option)
			if (num < '0' || num > '9')
				return false;

		auto convertedValue = std::stoull(option);

		return firstOption <= convertedValue && convertedValue <= lastOption;
	}
	int enterNumberInRange(std::string message, int firstOption, int lastOption) {
		bool isOptionVerified = false;
		std::string option;

		std::cout << "\n" << message;
		getline(std::cin, option);

		isOptionVerified = validateEnteredNumber(option, firstOption, lastOption);

		if (!isOptionVerified)
			std::cout << "\nПомилка: були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!\n\n";

		return isOptionVerified? stoi(option) : -1;
	}
	void readDataFromFile() {
		std::string filepath = FilesManager::getSessionsDirectory() + editor->currentSession->getName();
		std::string textFromFile = FilesManager::readSessionData(filepath);
		editor->currentText = new std::string(textFromFile);
	}

	void templateForMenusAboutSessions(int& choice, std::string action) {
		std::cout << "\nЯкий сеанс хочете " << action << ":\n";
		std::cout << "0. Назад\n";
		std::cout << "1. Останній\n";
		std::cout << "2. Найперший\n";
		std::cout << "3. За позицією\n";
		std::cout << "4. За іменем\n";
		std::cout << "5. Відсортувати сеанси за іменем\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 5);
	}
	void templateForExecutingMenusAboutSessions(std::function<void(int&)> mainFunc, std::function<void(int&)> menu, std::function<bool()> actionFuncByName, std::function<void()> additionalFunc = nullptr) {
		int choice, index = -1;
		bool isActionSuccessfull = false;

		do
		{
			editor->sessionsHistory->printSessionsHistory();
			menu(choice);
			switch (choice)
			{
			case -1: continue;
			case 0:
				std::cout << "\nПовернення до Головного меню.\n\n";
				system("pause");
				return;
			default:
				if (doesAnySessionExist())
				{
					switch (choice)
					{
					case 1:
					case 2:
					case 3:
						index = choice == 1 ? editor->sessionsHistory->size() : choice == 2 ? 1 : -1;
						mainFunc(index);
						if (index != -1 && additionalFunc)
							additionalFunc();
						continue;
					case 4:
						isActionSuccessfull = actionFuncByName();
						if(isActionSuccessfull && additionalFunc)
							additionalFunc();
						continue;
					case 5:
						editor->sessionsHistory->sortByName();
					}
				}
			}
		} while (true);
	}

	std::string getTextUsingKeyboard(std::string msg = "Введіть текст") {
		std::string line, text;
		int countOfLines = 0;

		std::cout << "\n" << msg << "(зупинити - з наступного рядка введіть - 1): \n";
		while (getline(std::cin, line)) {
			if (line == "-1")
				break;
			else {
				countOfLines++;
				if (countOfLines > 1)
					text += "\n" + line;
				else
					if (line == "")
						text += "\n";
					else
						text += line;
			}
		}

		return text;
	}
	std::string getTextFromClipboard() {
		if (editor->currentSession->sizeOfClipboard() == 0) {
			std::cout << "\nПомилка: в буфері обміну ще немає даних!\n\n";
			return "";
		}

		int choice, sizeOfClipboard = editor->currentSession->sizeOfClipboard();

		editor->currentSession->printClipboard();
		getTextFromClipboardMenu(choice);

		switch (choice)
		{
		case 0:
			std::cout << "\nПовернення до меню вибору способа додавання текста.\n\n";
			system("pause");
			return "";
		case 1:
			return editor->currentSession->getDataFromClipboardByIndex(sizeOfClipboard - 1);
		case 2:
			return editor->currentSession->getDataFromClipboardByIndex(0);
		case 3:
			choice = enterNumberInRange("Введіть номер даних: ", 1, sizeOfClipboard);
			if(choice != -1)
				return editor->currentSession->getDataFromClipboardByIndex(choice - 1);
			return "";
		default:
			return "";
		}
	}

	bool makeActionOnContextByEnteredText(std::string typeOfCommand, std::string actionInPast, size_t startIndex = -1, size_t endIndex = -1) {
		if(startIndex != -1 && endIndex != -1){
			std::string textForAction;

			textForAction = getTextUsingKeyboard("Введіть текст, над яким бажаєте зробити цю дію");

			if (textForAction.empty()) {
				std::cout << "\nПомилка: текст не був введений!\n\n";
				return false;
			}

			startIndex = editor->currentText->find(textForAction);

			if (startIndex == std::string::npos) {
				std::cout << "\nПомилка: текст не був знайдений!\n\n";
				return false;
			}

			if (textForAction.size() == 1)
				endIndex = startIndex;
			else
				endIndex = startIndex + textForAction.size() - 1;
		}

		commandsManager->invokeCommand(typeOfCommand, startIndex, endIndex);
		std::cout << "\nУспіх: дані були успішно " + actionInPast + "!\n\n";
		return true;
	}

	bool undoAction() {
		if (editor->currentSession->sizeOfCommandsHistory() > 0 && editor->currentSession->getCurIndexInCommHistory() != -1)
		{
			commandsManager->invokeCommand("Undo");
			std::cout << "\nУспіх: команда була успішно скасована!\n\n";
			return true;
		}
		std::cout << "\nПомилка: немає дій, які можна було б скасувати!\n\n";
		return false;
	}
	bool redoAction() {
		bool isThereAnyCommandForward = commandsManager->isThereAnyCommandForward();
		if (isThereAnyCommandForward)
		{
			commandsManager->invokeCommand("Redo");
			std::cout << "\nУспіх: команда була успішно повторена!\n\n";

		}
		else
			std::cout << "\nПомилка: немає дій, які можна було б повторити!\n\n";
		return isThereAnyCommandForward;
	}

	void wayToGetTextForAddingMenu(int& choice) {
		std::cout << "\nЯк ви хочете додати текст:\n";
		std::cout << "0. Назад\n";
		std::cout << "1. Ввівши з клавіатури\n";
		std::cout << "2. З буферу обміну\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 2);
	}
	void getTextFromClipboardMenu(int& choice) {
		std::cout << "\nЯкі дані бажаєте отримати з буферу обміну:\n";
		std::cout << "0. Назад\n";
		std::cout << "1. Останні\n";
		std::cout << "2. Найперші\n";
		std::cout << "3. За індексом\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 3);
	}
	void wayToPasteTextMenu(int& choice) {
		std::cout << "\nЯк ви хочете вставити текст:\n";
		std::cout << "0. Вийти в Меню дій над змістом\n";
		std::cout << "1. В кінець\n";
		std::cout << "2. На початок\n";
		std::cout << "3. Ввести з клавіатури текст, який хочете замінити\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 3);
	}
	void delCopyOrCutTextMenu(int& choice, std::string action) {
		std::cout << "\nСкільки хочете " << action << ":\n";
		std::cout << "0. Назад\n";
		std::cout << "1. Весь зміст\n";
		std::cout << "2. Введу з клавіатури, що " << action << "\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 2);
	}
	void makeActionsOnContentMenu(int& choice) {
		std::cout << "\nМеню дій над змістом:\n";
		std::cout << "0. Назад\n";
		std::cout << "1. Додати текст\n";
		std::cout << "2. Видалити текст\n";
		std::cout << "3. Копіювати текст\n";
		std::cout << "4. Вирізати текст\n";
		std::cout << "5. Скасувати команду\n";
		std::cout << "6. Повторити команду\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 6);
	}
	void printGettingSessionsMenu(int& choice) {
		templateForMenusAboutSessions(choice, "отримати");
	}
	void printDeletingSessionsMenu(int& choice) {
		templateForMenusAboutSessions(choice, "видалити");
	}
	void printManagingSessionsMenu(int& choice) {
		system("cls");
		std::cout << "Головне меню:\n";
		std::cout << "0. Закрити програму\n";
		std::cout << "1. Довідка\n";
		std::cout << "2. Створити сеанс\n";
		std::cout << "3. Відкрити сеанс\n";
		std::cout << "4. Видалити сеанс\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 4);
	}

	bool executeGettingTextForAdding(std::string& text) {
		int choice;

		do
		{
			editor->printCurrentText();
			wayToGetTextForAddingMenu(choice);

			switch (choice)
			{
			case 0:
				std::cout << "\nПовернення до Меню дій над змістом.\n\n";
				system("pause");
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
	bool executeAddingTextToFile() {
		std::string textToPaste;
		bool shallTextBeAdded = executeGettingTextForAdding(textToPaste);
		if (!shallTextBeAdded || textToPaste == "")
			return false;

		int choice;
		bool wasActionSuccessfull = false;
		editor->printCurrentText();
		wayToPasteTextMenu(choice);

		switch (choice) {
		case 0:
			std::cout << "\nПовернення до Меню дій над змістом.\n\n";
			system("pause");
			return false;
		case 1:
			makeActionOnContextByEnteredText("Paste", "вставлені", editor->currentText->size() - 1, editor->currentText->size() - 1);
			return true;
		case 2:
			makeActionOnContextByEnteredText("Paste", "вставлені", 0, 0);
			return true;
		case 3:
			wasActionSuccessfull = makeActionOnContextByEnteredText("Paste", "вставлені");
			return wasActionSuccessfull;
		default:
			return false;
		}
	}
	void executeMakeActionsOnContentMenu() {
		commandsManager = new CommandsManager(editor);
		bool wasTextSuccessfullyChanged;
		int choice;

		readDataFromFile();

		do
		{
			wasTextSuccessfullyChanged = false;
			editor->printCurrentText();
			makeActionsOnContentMenu(choice);

			switch (choice)
			{
			case 0:
				std::cout << "\nПовернення до Меню для отримання сеансу.\n\n";
				system("pause");
				delete (commandsManager);
				return;
			case 1:
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
				FilesManager::writeSessionData(FilesManager::getSessionsDirectory() + editor->currentSession->getName(), *(editor->currentText));
		} while (true);
	}
	void executeDeletingSessionsMenu() {
		std::function<void(int)> mainFunc = [this](int index) {
			deleteSessionByIndex(index);
			};
		std::function<void(int&)> menu = [this](int& choice) {
			printDeletingSessionsMenu(choice);
			};
		std::function<bool()> actionFuncByName = [this]() {
			return deleteSessionByName();
			};

		templateForExecutingMenusAboutSessions(mainFunc, menu, actionFuncByName);
	}
	void executeGettingSessionsMenu() {
		std::function<void(int&)> mainFunc = [this](int &index) {
			setCurrentSessionByIndex(index);
			};
		std::function<void(int&)> menu = [this](int& choice) {
			printGettingSessionsMenu(choice);
			};
		std::function<bool()> actionFuncByName = [this]() {
			return setCurrentSessionByName();
			};
		std::function<void()> additionalFunc = [this]() {
			executeMakeActionsOnContentMenu();
			};

		templateForExecutingMenusAboutSessions(mainFunc, menu, actionFuncByName, additionalFunc);
	}
	bool executeDelCopyOrCutText(std::string typeOfCommand, std::string actionForMenu, std::string actionInPast) {
		int choice;

		editor->printCurrentText();
		delCopyOrCutTextMenu(choice, actionForMenu);

		switch (choice)
		{
		case 0:
			std::cout << "\nПовернення до Меню дій над змістом.\n\n";
			system("pause");
			return false;
		case 1:
			return makeActionOnContextByEnteredText(typeOfCommand, actionInPast, 0, editor->currentText->size() - 1);
		case 2:
			return makeActionOnContextByEnteredText(typeOfCommand, actionInPast);
		}
	}
	bool chooseRootDelCopyOrCut(std::string action) {
		if (editor->currentText->size() == 0)
		{
			std::cout << "\nПомилка: немає тексту, який можна було б " + action + "!\n\n";
			return false;
		}
		else
		{
			if (action == "видалити")
				return executeDelCopyOrCutText("Delete", action, "видалені");
			else if (action == "скопіювати")
				return executeDelCopyOrCutText("Copy", action, "скопійовані");
			else
				return executeDelCopyOrCutText("Cut", action, "вирізані");
		}
	}

	bool tryToEnterIndexForSession(int& index) {
		if (index == -1) {
			index = enterNumberInRange("Введіть номер сеансу: ", 1, editor->sessionsHistory->size());
			if (index == -1)
				return false;
		}
		return true;
	}
	bool doesAnySessionExist() {
		if (editor->sessionsHistory->isEmpty())
			std::cout << "\nПомилка: в даний момент жодного сеансу немає!\n\n";
		return !editor->sessionsHistory->isEmpty();
	}

	void createSession() {
		std::string filename;

		std::cout << "\nВведіть ім'я сеансу (заборонені символи: /\\\":?*|<>): ";
		getline(std::cin, filename);

		Session* newSession = new Session();
		if (newSession->setName(filename))
		{
			if (editor->sessionsHistory->getSessionByName(filename) == nullptr) {
				delete newSession;
				std::cout << "\nПомилка: сеанс з таким іменем вже існує!\n\n";
				return;
			}

			if (!std::filesystem::exists(FilesManager::getSessionsDirectory()))
				std::filesystem::create_directories(FilesManager::getSessionsDirectory());

			std::string filepath = FilesManager::getSessionsDirectory() + newSession->getName();
			std::ofstream file(filepath);
			file.close();
			editor->sessionsHistory->addSessionToEnd(newSession);
			std::cout << "\nУспіх: сеанс був успішно створений!\n\n";
		}
		else
		{
			delete newSession;
			std::cout << "\nПомилка: були введені заборонені символи!\n\n";
		}
	}
	void setCurrentSessionByIndex(int& index) {
		if(tryToEnterIndexForSession(index))
			editor->currentSession = editor->sessionsHistory->getSessionByIndex(index - 1);
	}
	bool setCurrentSessionByName() {
		std::string name;
		std::cout << "\nВведіть ім'я сеансу: ";
		getline(std::cin, name);
		auto session = editor->sessionsHistory->getSessionByName(name);
		if (session == nullptr)
			std::cout << "\nПомилка: сеанса з таким іменем не існує!\n\n";
		else 
			editor->currentSession = session;
		return session != nullptr;
	}
	void deleteSessionByIndex(int index = -1) {
		if(tryToEnterIndexForSession(index)){
			std::string nameOfSession = editor->sessionsHistory->deleteSessionByIndex(index - 1);
			std::string pathToSession = FilesManager::getSessionsDirectory() + nameOfSession;
			remove(pathToSession.c_str());

			std::cout << "\nУспіх: сеанс був успішно видалений!\n\n";
		}
	}
	bool deleteSessionByName() {
		std::string name;
		std::cout << "\nВведіть ім'я сеансу: ";
		getline(std::cin, name);
		auto filename = editor->sessionsHistory->deleteSessionByName(name);
		if(filename.empty())
		{
			std::cout << "\nПомилка: сеанса з таким іменем не існує!\n\n";
			return false;
		}
		std::string pathToSession = FilesManager::getSessionsDirectory() + filename;
		remove(pathToSession.c_str());

		std::cout << "\nУспіх: сеанс був успішно видалений!\n\n";
		return true;
	}

	void printReferenceInfo() {
		system("cls");
		std::cout << "\nДовідка до програми:\n\n";
		std::cout << "Вашої уваги пропонується програму курсової роботи з об'єктно-орієнтованого програмування, побудована за допомогою патерну проектування \"Команда\".\n";
		std::cout << "За допомогою програми можна створювати, редагувати та видаляти текстові файли, використовуючи при цьому команди Копіювання, Вставка, Вирізання та Відміна операції.\n\n";
		std::cout << "Розробник: Бредун Денис Сергійович з групи ПЗ-21-1/9.\n\n";
		system("pause");
	}
public:

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
				std::cout << "\nДо побачення!\n";
				editor->tryToUnloadSessions();
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
					choice == 3 ? executeGettingSessionsMenu() :
						executeDeletingSessionsMenu();
			}

		} while (true);
	}
};

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	Program program;
	program.executeMainMenu();
}