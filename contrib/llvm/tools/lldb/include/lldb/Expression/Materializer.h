//===-- Materializer.h ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_Materializer_h
#define liblldb_Materializer_h

// C Includes
// C++ Includes
#include <memory>
#include <vector>

// Other libraries and framework includes
// Project includes
#include "lldb/Core/Error.h"
#include "lldb/Expression/IRMemoryMap.h"
#include "lldb/Symbol/TaggedASTType.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/lldb-private-types.h"

namespace lldb_private {

class Materializer {
public:
  Materializer();
  ~Materializer();

  class Dematerializer {
  public:
    Dematerializer()
        : m_materializer(nullptr), m_map(nullptr),
          m_process_address(LLDB_INVALID_ADDRESS) {}

    ~Dematerializer() { Wipe(); }

    void Dematerialize(Error &err, lldb::addr_t frame_top,
                       lldb::addr_t frame_bottom);

    void Wipe();

    bool IsValid() {
      return m_materializer && m_map &&
             (m_process_address != LLDB_INVALID_ADDRESS);
    }

  private:
    friend class Materializer;

    Dematerializer(Materializer &materializer, lldb::StackFrameSP &frame_sp,
                   IRMemoryMap &map, lldb::addr_t process_address)
        : m_materializer(&materializer), m_map(&map),
          m_process_address(process_address) {
      if (frame_sp) {
        m_thread_wp = frame_sp->GetThread();
        m_stack_id = frame_sp->GetStackID();
      }
    }

    Materializer *m_materializer;
    lldb::ThreadWP m_thread_wp;
    StackID m_stack_id;
    IRMemoryMap *m_map;
    lldb::addr_t m_process_address;
  };

  typedef std::shared_ptr<Dematerializer> DematerializerSP;
  typedef std::weak_ptr<Dematerializer> DematerializerWP;

  DematerializerSP Materialize(lldb::StackFrameSP &frame_sp, IRMemoryMap &map,
                               lldb::addr_t process_address, Error &err);

  class PersistentVariableDelegate {
  public:
    virtual ~PersistentVariableDelegate();
    virtual ConstString GetName() = 0;
    virtual void DidDematerialize(lldb::ExpressionVariableSP &variable) = 0;
  };

  uint32_t
  AddPersistentVariable(lldb::ExpressionVariableSP &persistent_variable_sp,
                        PersistentVariableDelegate *delegate, Error &err);
  uint32_t AddVariable(lldb::VariableSP &variable_sp, Error &err);
  uint32_t AddResultVariable(const CompilerType &type, bool is_lvalue,
                             bool keep_in_memory,
                             PersistentVariableDelegate *delegate, Error &err);
  uint32_t AddSymbol(const Symbol &symbol_sp, Error &err);
  uint32_t AddRegister(const RegisterInfo &register_info, Error &err);

  uint32_t GetStructAlignment() { return m_struct_alignment; }

  uint32_t GetStructByteSize() { return m_current_offset; }

  class Entity {
  public:
    Entity() : m_alignment(1), m_size(0), m_offset(0) {}

    virtual ~Entity() = default;

    virtual void Materialize(lldb::StackFrameSP &frame_sp, IRMemoryMap &map,
                             lldb::addr_t process_address, Error &err) = 0;
    virtual void Dematerialize(lldb::StackFrameSP &frame_sp, IRMemoryMap &map,
                               lldb::addr_t process_address,
                               lldb::addr_t frame_top,
                               lldb::addr_t frame_bottom, Error &err) = 0;
    virtual void DumpToLog(IRMemoryMap &map, lldb::addr_t process_address,
                           Log *log) = 0;
    virtual void Wipe(IRMemoryMap &map, lldb::addr_t process_address) = 0;

    uint32_t GetAlignment() { return m_alignment; }

    uint32_t GetSize() { return m_size; }

    uint32_t GetOffset() { return m_offset; }

    void SetOffset(uint32_t offset) { m_offset = offset; }

  protected:
    void SetSizeAndAlignmentFromType(CompilerType &type);

    uint32_t m_alignment;
    uint32_t m_size;
    uint32_t m_offset;
  };

private:
  uint32_t AddStructMember(Entity &entity);

  typedef std::unique_ptr<Entity> EntityUP;
  typedef std::vector<EntityUP> EntityVector;

  DematerializerWP m_dematerializer_wp;
  EntityVector m_entities;
  uint32_t m_current_offset;
  uint32_t m_struct_alignment;
};

} // namespace lldb_private

#endif // liblldb_Materializer_h
