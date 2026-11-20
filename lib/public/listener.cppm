export module fcopy.listener;

export import fcopy.types;

export namespace fcopy {
class IListener {
  public:
	virtual ~IListener() = default;

	IListener() = default;
	IListener(IListener const&) = default;
	IListener(IListener&&) = default;
	IListener& operator=(IListener const&) = default;
	IListener& operator=(IListener&&) = default;

	virtual void pre_file_copy(Entry const& entry) = 0;
	virtual void on_file_copy_progress(Progress const& progress) = 0;
};

class ProgressPrinterListener : public IListener {
  public:
	void pre_file_copy(Entry const& entry) override { m_path = entry.path.generic_string(); }
	void on_file_copy_progress(Progress const& progress) override;

  private:
	void clear_line();
	void populate_line(Progress const& progress);

	std::string m_path{};
	std::string m_line{};
};
} // namespace fcopy
